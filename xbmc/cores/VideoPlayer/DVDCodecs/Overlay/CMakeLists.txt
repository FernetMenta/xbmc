set(SOURCES DVDOverlayCodec.cpp
            DVDOverlayCodecFFmpeg.cpp
            DVDOverlayCodecSSA.cpp
            DVDOverlayCodecText.cpp
            DVDOverlayCodecTX3G.cpp
            contrib/cc_decoder708.cpp
            contrib/cc_decoder.c)

core_add_library(dvdoverlaycodecs)
add_dependencies(dvdoverlaycodecs ffmpeg)
