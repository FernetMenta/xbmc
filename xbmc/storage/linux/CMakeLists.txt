set(SOURCES DeviceKitDisksProvider.cpp
            UDisksProvider.cpp)

if(UDEV_FOUND)
  list(APPEND SOURCES UDevProvider.cpp)
endif()

core_add_library(storage_linux)
