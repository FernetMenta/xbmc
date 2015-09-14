set(SOURCES AddonBuiltins.cpp
            ApplicationBuiltins.cpp
            Builtins.cpp
            CECBuiltins.cpp
            GUIBuiltins.cpp
            GUIControlBuiltins.cpp
            GUIContainerBuiltins.cpp
            LibraryBuiltins.cpp
            OpticalBuiltins.cpp
            PictureBuiltins.cpp
            PlayerBuiltins.cpp
            ProfileBuiltins.cpp
            PVRBuiltins.cpp
            SkinBuiltins.cpp
            SystemBuiltins.cpp
            WeatherBuiltins.cpp)

core_add_library(interfaces_builtins)
add_dependencies(interfaces_builtins libcpluff)
