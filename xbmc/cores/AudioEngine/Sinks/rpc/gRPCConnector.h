#pragma once

#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include <atomic>
#include <memory>
#include <string>

class ISinkgRPC
{
public:
};

class CConnectorInternal;

class CRPCConnector : private CThread
{
public:
  CRPCConnector();
  virtual ~CRPCConnector();
  void Start();
  bool GetDeviceInfo(CAEDeviceInfo& info);

  // called from rpc interface
  bool HandleConfigReply(const void* msg);

protected:
  void Process() override;
  bool FindService();

  ISinkgRPC* m_sink = nullptr;
  std::string m_ip;
  unsigned int m_port = 0;
  CCriticalSection m_section;
  std::atomic_bool m_abort = {false};

  CAEDeviceInfo m_deviceInfo;
  unsigned int m_latency;

  struct delete_connectorInternal
  {
    void operator()(CConnectorInternal *p) const;
  };
  std::unique_ptr<CConnectorInternal, delete_connectorInternal> m_internal;
};
