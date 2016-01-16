set(SOURCES AutorunMediaJob.cpp
            cdioSupport.cpp
            DetectDVDType.cpp
            IoSupport.cpp
            MediaManager.cpp)

core_add_library(storage)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(storage ffmpeg)
endif()

if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
  add_dependencies(storage dvdnav)
endif()
