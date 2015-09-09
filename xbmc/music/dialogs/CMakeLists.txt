set(SOURCES GUIDialogMusicInfo.cpp
            GUIDialogMusicOSD.cpp
            GUIDialogSongInfo.cpp
            GUIDialogVisualisationPresetList.cpp)

core_add_library(music_dialogs)
add_dependencies(music_dialogs libcpluff)
