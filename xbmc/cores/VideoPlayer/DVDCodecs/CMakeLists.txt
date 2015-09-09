set(SOURCES DVDCodecUtils.cpp
            DVDFactoryCodec.cpp)

core_add_library(dvdcodecs)
add_dependencies(dvdcodecs ffmpeg)
