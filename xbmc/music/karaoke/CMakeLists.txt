set(SOURCES GUIDialogKaraokeSongSelector.cpp
            GUIWindowKaraokeLyrics.cpp
            karaokelyricscdg.cpp
            karaokelyrics.cpp
            karaokelyricsfactory.cpp
            karaokelyricsmanager.cpp
            karaokelyricstext.cpp
            karaokelyricstextkar.cpp
            karaokelyricstextlrc.cpp
            karaokelyricstextustar.cpp
            karaokevideobackground.cpp
            karaokewindowbackground.cpp)

core_add_library(music_karaoke)
add_dependencies(music_karaoke ffmpeg)
