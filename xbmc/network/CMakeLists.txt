set(SOURCES cddb.cpp
            DNSNameCache.cpp
            EventClient.cpp
            EventPacket.cpp
            EventServer.cpp
            GUIDialogAccessPoints.cpp
            GUIDialogNetworkSetup.cpp
            Network.cpp
            NetworkServices.cpp
            Socket.cpp
            TCPServer.cpp
            UdpClient.cpp
            WakeOnAccess.cpp
            WebServer.cpp
            ZeroconfBrowser.cpp
            Zeroconf.cpp)

if(PLIST_FOUND)
  list(APPEND SOURCES AirPlayServer.cpp)
endif()

if(SHAIRPLAY_FOUND OR SHAIRPORT_FOUND)
  list(APPEND SOURCES AirTunesServer.cpp)
endif()

core_add_library(network)
add_dependencies(network ffmpeg)
