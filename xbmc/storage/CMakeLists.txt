set(SOURCES AutorunMediaJob.cpp
            cdioSupport.cpp
            DetectDVDType.cpp
            IoSupport.cpp
            MediaManager.cpp)

core_add_library(storage)
add_dependencies(storage dvdnav ffmpeg)
