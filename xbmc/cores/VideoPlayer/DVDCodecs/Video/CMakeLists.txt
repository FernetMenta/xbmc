set(SOURCES DVDVideoCodec.cpp
            DVDVideoCodecFFmpeg.cpp
            DVDVideoCodecLibMpeg2.cpp)

if(NOT ENABLE_EXTERNAL_LIBAV)
  list(APPEND SOURCES DVDVideoPPFFmpeg.cpp)
endif()

if(AMLCODEC_FOUND)
  list(APPEND SOURCES AMLCodec.cpp
                      DVDVideoCodecAmlogic.cpp)
endif()

if(OPENMAX_FOUND)
  list(APPEND SOURCES DVDVideoCodecOpenMax.cpp
                      OpenMax.cpp
                      OpenMaxVideo.cpp)
endif()

if(DXVA_FOUND)
  list(APPEND SOURCES DXVA.cpp)
endif()

if(VDPAU_FOUND)
  list(APPEND SOURCES VDPAU.cpp)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES VAAPI.cpp)
endif()

if(VDA_FOUND)
  list(APPEND SOURCES DVDVideoCodecVDA.cpp
                      VDA.cpp)
endif()

if(VTB_FOUND)
  list(APPEND SOURCES DVDVideoCodecVideoToolBox.cpp)
endif()

core_add_library(dvdvideocodecs)
add_dependencies(dvdvideocodecs ffmpeg)
