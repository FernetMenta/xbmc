set(SOURCES main.cpp)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
  list(APPEND SOURCES MessagePrinter.cpp)
endif()

core_add_library(platform_posix)
