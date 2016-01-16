set(SOURCES MusicInfoTag.cpp
            MusicInfoTagLoaderCDDA.cpp
            MusicInfoTagLoaderDatabase.cpp
            MusicInfoTagLoaderFactory.cpp
            MusicInfoTagLoaderFFmpeg.cpp
            MusicInfoTagLoaderShn.cpp
            ReplayGain.cpp
            TagLibVFSStream.cpp
            TagLoaderTagLib.cpp)

core_add_library(music_tags)

add_dependencies(music_tags libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(music_tags ffmpeg)
endif()
