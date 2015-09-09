set(SOURCES DVDAudio.cpp
            DVDClock.cpp
            DVDDemuxSPU.cpp
            DVDFileInfo.cpp
            DVDMessage.cpp
            DVDMessageQueue.cpp
            DVDOverlayContainer.cpp
            VideoPlayerAudio.cpp
            VideoPlayer.cpp
            VideoPlayerSubtitle.cpp
            VideoPlayerTeletext.cpp
            VideoPlayerVideo.cpp
            DVDStreamInfo.cpp
            DVDTSCorrection.cpp
            Edl.cpp)

core_add_library(dvdplayer)
add_dependencies(dvdplayer libcpluff ffmpeg dvdnav)
