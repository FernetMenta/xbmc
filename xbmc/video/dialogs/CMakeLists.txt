set(SOURCES GUIDialogAudioSubtitleSettings.cpp
            GUIDialogFullScreenInfo.cpp
            GUIDialogSubtitles.cpp
            GUIDialogTeletext.cpp
            GUIDialogVideoBookmarks.cpp
            GUIDialogVideoInfo.cpp
            GUIDialogVideoOSD.cpp
            GUIDialogVideoSettings.cpp)

core_add_library(video_dialogs)

add_dependencies(video_dialogs libcpluff ffmpeg)
