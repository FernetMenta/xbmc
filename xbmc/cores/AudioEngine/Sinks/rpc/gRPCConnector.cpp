
#include "gRPCConnector.h"


#include "isound.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class CConnectorInternal
{
public:
  CConnectorInternal(std::shared_ptr<Channel> channel)
    : m_stub(Greeter::NewStub(channel)) {};


  std::string SayHello(const std::string& user)
  {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = m_stub->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok())
    {
      return reply.message();
    }
    else
    {
      std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
      return "RPC failed";
    }
  }

  std::unique_ptr<Greeter::Stub> m_stub;
};

CRPCConnector::CRPCConnector()
{
  m_internal.reset(new CConnectorInternal(grpc::CreateChannel(
                                                "localhost:50051",
                                                grpc::InsecureChannelCredentials())));
}

std::string CRPCConnector::SayHello(const std::string& user)
{
  return m_internal->SayHello(user);
}

// deleters for unique_ptr
void CRPCConnector::delete_connectorInternal::operator()(CConnectorInternal *p) const
{
  delete p;
}
