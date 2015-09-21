set(SOURCES GUIWindowMusicBase.cpp
            GUIWindowMusicNav.cpp
            GUIWindowMusicPlaylist.cpp
            GUIWindowMusicPlaylistEditor.cpp
            GUIWindowVisualisation.cpp)

core_add_library(music_windows)
add_dependencies(music_windows libcpluff ffmpeg)
