set(SOURCES DataCacheCore.cpp
            FFmpeg.cpp)

core_add_library(cores)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(cores ffmpeg)
endif()
