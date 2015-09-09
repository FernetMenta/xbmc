SET(SOURCES "")

if(UDEV_FOUND)
  list(APPEND SOURCES PeripheralBusUSBLibUdev.cpp)
else()
  if(LibUSB_FOUND)
    list(APPEND SOURCES PeripheralBusUSBLibUSB.cpp)
  endif()
endif()

core_add_library(peripheral_bus_linux)
