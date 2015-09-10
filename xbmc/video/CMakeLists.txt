set(SOURCES Bookmark.cpp
            FFmpegVideoDecoder.cpp
            GUIViewStateVideo.cpp
            PlayerController.cpp
            Teletext.cpp
            VideoDatabase.cpp
            VideoDbUrl.cpp
            VideoInfoDownloader.cpp
            VideoInfoScanner.cpp
            VideoInfoTag.cpp
            VideoLibraryQueue.cpp
            VideoReferenceClock.cpp
            VideoThumbLoader.cpp)

core_add_library(video)
add_dependencies(video libcpluff ffmpeg)
