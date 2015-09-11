set(SOURCES AddonEvent.cpp
            AddonManagementEvent.cpp
            BaseEvent.cpp
            EventLog.cpp
            MediaLibraryEvent.cpp)

core_add_library(events)
add_dependencies(events libcpluff)
