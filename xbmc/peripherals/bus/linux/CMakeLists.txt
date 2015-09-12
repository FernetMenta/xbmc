SET(SOURCES "")

if(UDEV_FOUND)
  list(APPEND SOURCES PeripheralBusUSBLibUdev.cpp)
else()
  if(LibUSB_FOUND)
    list(APPEND SOURCES PeripheralBusUSBLibUSB.cpp)
  endif()
endif()

core_add_library(peripherals_bus_linux)
set_target_properties(peripherals_bus_linux PROPERTIES LINKER_LANGUAGE CXX)
