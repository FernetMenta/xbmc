set(SOURCES)
if(X_FOUND)
  list(APPEND SOURCES VideoSyncDRM.cpp)
endif()
if(X_FOUND AND OPENGL_FOUND)
  list(APPEND SOURCES VideoSyncGLX.cpp)
endif()

core_add_library(video_sync)
