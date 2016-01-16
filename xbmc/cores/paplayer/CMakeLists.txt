set(SOURCES AudioDecoder.cpp
            CodecFactory.cpp
            PAPlayer.cpp
            VideoPlayerCodec.cpp)

core_add_library(paplayer)
add_dependencies(paplayer libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(paplayer ffmpeg)
endif()
