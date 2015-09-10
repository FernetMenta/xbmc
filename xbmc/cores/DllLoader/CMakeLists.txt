set(SOURCES coff.cpp
            dll.cpp
            DllLoader.cpp
            DllLoaderContainer.cpp
            dll_tracker.cpp
            dll_tracker_file.cpp
            dll_tracker_library.cpp
            dll_util.cpp
            LibraryLoader.cpp
            mmap_anon.c
            SoLoader.cpp)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  list(APPEND SOURCES ldt_keeper.c)
endif()

add_definitions(-DAPI_DEBUG)

core_add_library(dllloader)
