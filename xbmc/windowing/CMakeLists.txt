set(SOURCES WinEvents.cpp
            WinSystem.cpp)

if(SDL_FOUND AND NOT X_FOUND)
  list(APPEND SOURCES WinEventsSDL.cpp)
endif()

if(X_FOUND)
  list(APPEND SOURCES WinEventsX11.cpp)
endif()


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  list(APPEND SOURCES WinEventsLinux.cpp)
endif()

core_add_library(windowing)
