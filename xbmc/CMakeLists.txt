set(SOURCES Application.cpp
            ApplicationPlayer.cpp
            AppParamParser.cpp
            Autorun.cpp
            AutoSwitch.cpp
            BackgroundInfoLoader.cpp
            ContextMenuItem.cpp
            ContextMenuManager.cpp
            CueDocument.cpp
            DatabaseManager.cpp
            DbUrl.cpp
            DynamicDll.cpp
            FileItem.cpp
            FileItemListModification.cpp
            GUIInfoManager.cpp
            GUILargeTextureManager.cpp
            GUIPassword.cpp
            LangInfo.cpp
            MediaSource.cpp
            NfoFile.cpp
            PasswordManager.cpp
            PlayListPlayer.cpp
            PartyModeManager.cpp
            SectionLoader.cpp
            SystemGlobals.cpp
            TextureCache.cpp
            TextureCacheJob.cpp
            TextureDatabase.cpp
            ThumbLoader.cpp
            ThumbnailCache.cpp
            URL.cpp
            Util.cpp
            XBApplicationEx.cpp
            XBDateTime.cpp)

core_add_library(xbmc)
add_dependencies(xbmc libcpluff ffmpeg)
