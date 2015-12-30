set(SOURCES coff.cpp
            dll.cpp
            DllLoader.cpp
            DllLoaderContainer.cpp
            dll_tracker.cpp
            dll_tracker_file.cpp
            dll_tracker_library.cpp
            dll_util.cpp
            LibraryLoader.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND SOURCES mmap_anon.c
                      ldt_keeper.c
                      SoLoader.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list(APPEND SOURCES Win32DllLoader.cpp)
endif()

add_definitions(-DAPI_DEBUG)

core_add_library(dllloader)
