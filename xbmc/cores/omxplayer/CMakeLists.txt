set(SOURCES OMXHelper.cpp
            OMXAudio.cpp
            OMXVideo.cpp
            OMXAudioCodecOMX.cpp
            OMXPlayerAudio.cpp
            OMXPlayerVideo.cpp
            OMXImage.cpp)

core_add_library(omxplayer)
add_definitions(-D__STDC_FORMAT_MACROS)

add_dependencies(omxplayer linuxsupport ffmpeg)


