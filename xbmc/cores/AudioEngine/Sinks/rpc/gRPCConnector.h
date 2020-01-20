
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

class CConnectorInternal;

class CRPCConnector
{
public:
  CRPCConnector();
  std::string SayHello(const std::string& user);

protected:
  struct delete_connectorInternal
  {
    void operator()(CConnectorInternal *p) const;
  };
  std::unique_ptr<CConnectorInternal, delete_connectorInternal> m_internal;
};
