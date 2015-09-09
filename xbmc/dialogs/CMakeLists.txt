set(SOURCES GUIDialogBoxBase.cpp
            GUIDialogBusy.cpp
            GUIDialogButtonMenu.cpp
            GUIDialogCache.cpp
            GUIDialogContextMenu.cpp
            GUIDialogExtendedProgressBar.cpp
            GUIDialogFavourites.cpp
            GUIDialogFileBrowser.cpp
            GUIDialogGamepad.cpp
            GUIDialogKaiToast.cpp
            GUIDialogKeyboardGeneric.cpp
            GUIDialogMediaFilter.cpp
            GUIDialogMediaSource.cpp
            GUIDialogMuteBug.cpp
            GUIDialogNumeric.cpp
            GUIDialogOK.cpp
            GUIDialogPlayEject.cpp
            GUIDialogPlayerControls.cpp
            GUIDialogProgress.cpp
            GUIDialogSeekBar.cpp
            GUIDialogSelect.cpp
            GUIDialogSimpleMenu.cpp
            GUIDialogSlider.cpp
            GUIDialogSmartPlaylistEditor.cpp
            GUIDialogSmartPlaylistRule.cpp
            GUIDialogSubMenu.cpp
            GUIDialogTextViewer.cpp
            GUIDialogVolumeBar.cpp
            GUIDialogYesNo.cpp)

core_add_library(dialogs)
add_dependencies(dialogs libcpluff)
