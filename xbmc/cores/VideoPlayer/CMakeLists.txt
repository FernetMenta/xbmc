set(SOURCES DVDAudio.cpp
            DVDClock.cpp
            DVDDemuxSPU.cpp
            DVDFileInfo.cpp
            DVDMessage.cpp
            DVDMessageQueue.cpp
            DVDOverlayContainer.cpp
            VideoPlayerAudio.cpp
            VideoPlayer.cpp
            VideoPlayerRadioRDS.cpp
            VideoPlayerSubtitle.cpp
            VideoPlayerTeletext.cpp
            VideoPlayerVideo.cpp
            DVDStreamInfo.cpp
            DVDTSCorrection.cpp
            Edl.cpp)

core_add_library(VideoPlayer)
add_dependencies(VideoPlayer libcpluff ffmpeg)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
  add_dependencies(VideoPlayer dvdnav)
endif()
