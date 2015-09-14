set(SOURCES GUIDialogPVRChannelManager.cpp
            GUIDialogPVRChannelsOSD.cpp
            GUIDialogPVRGroupManager.cpp
            GUIDialogPVRGuideInfo.cpp
            GUIDialogPVRGuideOSD.cpp
            GUIDialogPVRGuideSearch.cpp
            GUIDialogPVRRadioRDSInfo.cpp
            GUIDialogPVRRecordingInfo.cpp
            GUIDialogPVRTimerSettings.cpp)

core_add_library(pvr_dialogs)
add_dependencies(pvr_dialogs libcpluff)
