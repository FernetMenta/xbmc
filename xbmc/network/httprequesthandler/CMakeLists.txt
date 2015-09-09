set(SOURCES HTTPFileHandler.cpp
            HTTPImageHandler.cpp
            HTTPImageTransformationHandler.cpp
            HTTPJsonRpcHandler.cpp
            HTTPPythonHandler.cpp
            HTTPVfsHandler.cpp
            HTTPWebinterfaceAddonsHandler.cpp
            HTTPWebinterfaceHandler.cpp
            IHTTPRequestHandler.cpp)

core_add_library(network_httprequesthandlers)
add_dependencies(network_httprequesthandlers libcpluff)
