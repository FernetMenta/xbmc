set(SOURCES PVRActionListener.cpp
            PVRGUIInfo.cpp
            PVRManager.cpp
            PVRDatabase.cpp)

core_add_library(pvr)
add_dependencies(pvr libcpluff)
