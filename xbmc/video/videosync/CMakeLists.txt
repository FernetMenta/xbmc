set(SOURCES)
if(CORE_SYSTEM_NAME STREQUAL "rbpi")
  list(APPEND SOURCES VideoSyncPi.cpp)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES VideoSyncGLX.cpp)
endif()

if(X_FOUND)
  list(APPEND SOURCES VideoSyncDRM.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "windows")
  list(APPEND SOURCES VideoSyncD3D.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "darwin")
  list(APPEND SOURCES VideoSyncOsx.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "ios")
  list(APPEND SOURCES VideoSyncIos.cpp)
endif()

core_add_library(video_sync)
