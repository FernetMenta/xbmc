set(SOURCES Peripheral.cpp
            PeripheralBluetooth.cpp
            PeripheralDisk.cpp
            PeripheralHID.cpp
            PeripheralImon.cpp
            PeripheralNIC.cpp
            PeripheralNyxboard.cpp
            PeripheralTuner.cpp)

if(CEC_FOUND)
  list(APPEND SOURCES PeripheralCecAdapter.cpp)
endif()

core_add_library(peripherals_devices)
