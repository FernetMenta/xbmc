if(CORE_SYSTEM_NAME STREQUAL "darwin")
  set(SOURCES WinEventsOSX.mm
              WinSystemOSX.mm
              WinSystemOSXGL.mm)
endif()

if(CORE_SYSTEM_NAME STREQUAL "ios")
  set(SOURCES WinEventsIOS.mm
              WinSystemIOS.mm)
endif()

core_add_library(windowing_osx)
