set(SOURCES GUIWindowFullScreen.cpp
            GUIWindowVideoBase.cpp
            GUIWindowVideoNav.cpp
            GUIWindowVideoPlaylist.cpp
            VideoFileItemListModifier)

core_add_library(video_windows)
add_dependencies(video_windows libcpluff)
