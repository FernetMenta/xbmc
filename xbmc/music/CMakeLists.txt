set(SOURCES Album.cpp
            Artist.cpp
            CueInfoLoader.cpp
            GUIViewStateMusic.cpp
            MusicDatabase.cpp
            MusicDbUrl.cpp
            MusicInfoLoader.cpp
            MusicThumbLoader.cpp
            Song.cpp)

core_add_library(music)
add_dependencies(music libcpluff)
