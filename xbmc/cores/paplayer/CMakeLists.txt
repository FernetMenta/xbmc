set(SOURCES AudioDecoder.cpp
            CodecFactory.cpp
            PAPlayer.cpp
            VideoPlayerCodec.cpp)

core_add_library(paplayer)
add_dependencies(paplayer libcpluff ffmpeg)
