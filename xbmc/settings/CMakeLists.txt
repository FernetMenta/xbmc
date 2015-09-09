set(SOURCES AdvancedSettings.cpp
            AudioDSPSettings.cpp
            DisplaySettings.cpp
            MediaSettings.cpp
            MediaSourceSettings.cpp
            SettingAddon.cpp
            SettingConditions.cpp
            SettingControl.cpp
            SettingCreator.cpp
            SettingPath.cpp
            Settings.cpp
            SettingUtils.cpp
            SkinSettings.cpp
            VideoSettings.cpp)

core_add_library(settings)
add_dependencies(settings libcpluff ffmpeg)
