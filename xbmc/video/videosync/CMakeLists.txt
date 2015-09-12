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

core_add_library(video_sync)
