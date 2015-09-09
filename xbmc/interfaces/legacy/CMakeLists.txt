set(SOURCES AddonCallback.cpp
            AddonClass.cpp
            Addon.cpp
            AddonUtils.cpp
            CallbackFunction.cpp
            CallbackHandler.cpp
            Control.cpp
            Dialog.cpp
            File.cpp
            InfoTagMusic.cpp
            InfoTagVideo.cpp
            Keyboard.cpp
            LanguageHook.cpp
            ListItem.cpp
            ModuleXbmc.cpp
            ModuleXbmcgui.cpp
            ModuleXbmcplugin.cpp
            ModuleXbmcvfs.cpp
            Monitor.cpp
            Player.cpp
            PlayList.cpp
            String.cpp
            Window.cpp
            WindowDialog.cpp
            WindowDialogMixin.cpp
            WindowXML.cpp)

core_add_library(legacy_interface)
add_dependencies(legacy_interface libcpluff)
