set(SOURCES GUIViewStatePVR.cpp
            GUIWindowPVRBase.cpp
            GUIWindowPVRChannels.cpp
            GUIWindowPVRGuide.cpp
            GUIWindowPVRRecordings.cpp
            GUIWindowPVRSearch.cpp
            GUIWindowPVRTimers.cpp
            GUIWindowPVRTimersBase.cpp
            GUIWindowPVRTimerRules.cpp)

core_add_library(pvr_windows)
add_dependencies(pvr_windows libcpluff ffmpeg)
