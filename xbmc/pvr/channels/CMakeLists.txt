set(SOURCES PVRChannel.cpp
            PVRChannelGroup.cpp
            PVRChannelGroupInternal.cpp
            PVRChannelGroups.cpp
            PVRChannelGroupsContainer.cpp
            PVRRadioRDSInfoTag.cpp)

core_add_library(pvr_channels)
add_dependencies(pvr_channels libcpluff)
