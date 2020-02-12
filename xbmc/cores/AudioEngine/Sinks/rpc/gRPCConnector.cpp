
#include "gRPCConnector.h"

#include "source.grpc.pb.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "network/ZeroconfBrowser.h"
#include "utils/log.h"
#include <grpcpp/grpcpp.h>
#include <string>
#include <iostream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using audiosource::ServerToClient;
using audiosource::ClientToServer;
using audiosource::Audiosource;
using audiosource::ClockRequest;
using audiosource::ClockReply;
using audiosource::ConfigRequest;
using audiosource::ConfigReply;
using audiosource::Error;

//------------------------------------------------------------------------------------
// gRPC connector
//------------------------------------------------------------------------------------

class CConnectorInternal
{
public:
  CConnectorInternal(std::shared_ptr<Channel> channel, CCriticalSection& section, CRPCConnector* connector, std::atomic_bool& abort)
    : m_stub(Audiosource::NewStub(channel)), m_section(section), m_abort(abort)
  {
    m_connector = connector;
  };

  void Connect()
  {
    {
      CSingleLock lock(m_section);
      if (m_abort)
        return;
      m_stream = m_stub->MsgStream(&m_context);
      m_connected = true;
      RequestConfig();
    }

    ServerToClient msg;
    while (!m_abort && m_stream->Read(&msg))
    {
      CSingleLock lock(m_section);
      switch (msg.server_oneof_case())
      {
        case ServerToClient::kConfigreply:
          m_connector->HandleConfigReply(&msg.configreply());
          break;
        default:
          CLog::Log(LOGERROR, "MsgStreamServiceImpl::MsgStream - no case set for message");
          break;
      }
    }

    m_connected = false;
    CSingleLock lock(m_section);
    m_stream.reset();
  }

  bool IsConnected()
  {
    CSingleLock lock(m_section);
    return m_connected;
  }

  void Terminate()
  {
    m_abort = true;
    CSingleLock lock(m_section);
    if (m_connected)
    {
      m_stream->WritesDone();
      m_stream->Finish();
    }
  }

  bool RequestConfig()
  {
    CSingleLock lock(m_section);

    if (!m_connected)
      return false;

    ClientToServer msg;
    ConfigRequest* request = msg.mutable_configrequest();
    request->set_client_name("Kodi");

    if (!m_stream->Write(msg))
      return false;

    return true;
  }

protected:
  std::unique_ptr<Audiosource::Stub> m_stub;
  std::shared_ptr<ClientReaderWriter<ClientToServer, ServerToClient>> m_stream;
  ClientContext m_context;
  std::atomic_bool m_connected = {false};
  CCriticalSection& m_section;
  std::atomic_bool& m_abort;
  CRPCConnector* m_connector = nullptr;
};

//------------------------------------------------------------------------------------
// Connector
//------------------------------------------------------------------------------------

CRPCConnector::CRPCConnector() : CThread("AE-gRPC")
{
}

CRPCConnector::~CRPCConnector()
{
  {
    CSingleLock lock(m_section);
    if (m_internal)
      m_internal->Terminate();
  }
  StopThread();
}

void CRPCConnector::Start()
{
  m_abort = false;
  Create();
}

bool CRPCConnector::GetDeviceInfo(CAEDeviceInfo& info)
{
  CSingleLock lock(m_section);
  if (!m_internal || !m_internal->IsConnected() || m_deviceInfo.m_deviceName.empty())
    return false;

  info = m_deviceInfo;
  return true;
}

void CRPCConnector::Process()
{
  CZeroconfBrowser::GetInstance()->Start();
  CZeroconfBrowser::GetInstance()->AddServiceType("_soundserver._tcp");

  while (!m_bStop && !m_abort)
  {
    if (!FindService())
    {
      Sleep(1000);
      continue;
    }

    m_deviceInfo.m_deviceName.clear();

    std::string address = m_ip + ":" + std::to_string(m_port);

    {
      CSingleLock lock(m_section);
      m_internal.reset(new CConnectorInternal(grpc::CreateChannel(
                                              address,
                                              grpc::InsecureChannelCredentials()),
                                              m_section, this, m_abort));
    }

    m_internal->Connect();

    if (!m_bStop)
      Sleep(1000);
  }

  CZeroconfBrowser::GetInstance()->RemoveServiceType("_soundserver._tcp");
  CZeroconfBrowser::GetInstance()->Stop();
}

bool CRPCConnector::FindService()
{
  std::vector<CZeroconfBrowser::ZeroconfService> services = CZeroconfBrowser::GetInstance()->GetFoundServices();
  for (auto service : services)
  {
    if (StringUtils::EqualsNoCase(service.GetType(), std::string("_soundserver._tcp") + "."))
    {
      if (StringUtils::StartsWithNoCase(service.GetName(), "Soundserver"))
      {
        // resolve the service and save it
        CZeroconfBrowser::GetInstance()->ResolveService(service);
        m_ip = service.GetIP();
        m_port = service.GetPort();
        return true;
      }
    }
  }
  return false;
}

// deleters for unique_ptr
void CRPCConnector::delete_connectorInternal::operator()(CConnectorInternal *p) const
{
  delete p;
}

//------------------------------------------------------------------------------------
// called from external interface
//------------------------------------------------------------------------------------

bool CRPCConnector::HandleConfigReply(const void* msg)
{
  const ConfigReply* reply = static_cast<const ConfigReply*>(msg);

  if (reply->config_size() < 1)
    return false;

  const ConfigReply::Config& config = reply->config(0);

  // channels
  std::stringstream sStream(config.channels());
  std::vector<std::string> sChannels;
  while(sStream.good())
  {
    std::string substr;
    std::getline(sStream, substr, ',');
    substr.erase(0, substr.find_first_not_of("\t\n\v\f\r "));
    substr.erase(substr.find_last_not_of("\t\n\v\f\r ") + 1);
    sChannels.push_back(substr);
  }
  CAEChannelInfo channels;
  for (auto chan : sChannels)
  {
    if (chan == "FL")
      channels += AE_CH_FL;
    else if (chan == "FR")
      channels += AE_CH_FR;
  }

  // sample rates
  sStream = std::stringstream(config.samplerates());
  std::vector<unsigned int> samplerates;
  while(sStream.good())
  {
    std::string substr;
    std::getline(sStream, substr, ',');
    substr.erase(0, substr.find_first_not_of("\t\n\v\f\r "));
    substr.erase(substr.find_last_not_of("\t\n\v\f\r ") + 1);
    samplerates.push_back(std::stoi(substr));
  }

  // data formats
  sStream = std::stringstream(config.datatypes());
  std::vector<std::string> sDatatypes;
  while(sStream.good())
  {
    std::string substr;
    std::getline(sStream, substr, ',');
    substr.erase(0, substr.find_first_not_of("\t\n\v\f\r "));
    substr.erase(substr.find_last_not_of("\t\n\v\f\r ") + 1);
    sDatatypes.push_back(substr);
  }
  AEDataFormatList dataformats;
  for (auto type : sDatatypes)
  {
    if (type == "INT16")
      dataformats.push_back(config.interleaved() ? AE_FMT_S16NE : AE_FMT_S16NEP);
    else if (type == "INT32")
      dataformats.push_back(config.interleaved() ? AE_FMT_S32NE : AE_FMT_S32NEP);
  }

  bool change = false;
  if (m_deviceInfo.m_channels != channels ||
      m_deviceInfo.m_dataFormats != dataformats ||
      m_deviceInfo.m_sampleRates != samplerates)
    change = true;

  m_deviceInfo.m_channels = channels;
  m_deviceInfo.m_dataFormats = dataformats;
  m_deviceInfo.m_sampleRates = samplerates;
  m_deviceInfo.m_deviceName = config.name();
  m_latency = config.latency();

  if (change)
  {
    CLog::Log(LOGDEBUG, "gRPC Sink: config changed");
    IAE* ae = CServiceBroker::GetActiveAE();
    if (ae)
      ae->DeviceChange();
  }

  return true;
}
