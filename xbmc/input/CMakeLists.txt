set(SOURCES ButtonTranslator.cpp
            InertialScrollingHandler.cpp
            InputCodingTableBaiduPY.cpp
            InputCodingTableBasePY.cpp
            InputCodingTableFactory.cpp
            InputManager.cpp
            Key.cpp
            KeyboardLayout.cpp
            KeyboardLayoutManager.cpp
            KeyboardStat.cpp
            MouseStat.cpp
            XBMC_keytable.cpp)

if(SDL_FOUND)
  list(APPEND SOURCES SDLJoystick.cpp)
endif()

core_add_library(input)
