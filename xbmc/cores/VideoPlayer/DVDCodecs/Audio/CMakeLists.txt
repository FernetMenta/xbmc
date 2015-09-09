set(SOURCES DVDAudioCodecFFmpeg.cpp
            DVDAudioCodecPassthrough.cpp)

core_add_library(dvdaudiocodecs)
add_dependencies(dvdaudiocodecs ffmpeg)
