set(SOURCES DVDOverlayCodec.cpp
            DVDOverlayCodecFFmpeg.cpp
            DVDOverlayCodecSSA.cpp
            DVDOverlayCodecText.cpp
            DVDOverlayCodecTX3G.cpp
            contrib/cc_decoder708.cpp
            contrib/cc_decoder.c)

if(CORE_SYSTEM_NAME STREQUAL "windows")
  set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif()

core_add_library(dvdoverlaycodecs)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvdoverlaycodecs ffmpeg)
endif()
