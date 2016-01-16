set(SOURCES DVDAudioCodecFFmpeg.cpp
            DVDAudioCodecPassthrough.cpp)

core_add_library(dvdaudiocodecs)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvdaudiocodecs ffmpeg)
endif()
