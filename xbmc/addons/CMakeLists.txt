set(SOURCES AddonCallbacksAddon.cpp
            AddonCallbacksAudioDSP.cpp
            AddonCallbacksCodec.cpp
            AddonCallbacks.cpp
            AddonCallbacksGUI.cpp
            AddonCallbacksPVR.cpp
            Addon.cpp
            AddonDatabase.cpp
            AddonInstaller.cpp
            AddonManager.cpp
            AddonStatusHandler.cpp
            AddonVersion.cpp
            AudioDecoder.cpp
            AudioEncoder.cpp
            ContextMenuAddon.cpp
            GUIDialogAddonInfo.cpp
            GUIDialogAddonSettings.cpp
            GUIViewStateAddonBrowser.cpp
            GUIWindowAddonBrowser.cpp
            ImageResource.cpp
            LanguageResource.cpp
            PluginSource.cpp
            Repository.cpp
            RepositoryUpdater.cpp
            Scraper.cpp
            ScreenSaver.cpp
            Service.cpp
            Skin.cpp
            UISoundsResource.cpp
            Visualisation.cpp
            Webinterface.cpp)

core_add_library(addons)
add_dependencies(addons libcpluff ffmpeg)
