set(SOURCES DVDCodecUtils.cpp
            DVDFactoryCodec.cpp)

core_add_library(dvdcodecs)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvdcodecs ffmpeg)
endif()
