set(SOURCES libKODI_guilib.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CORE_SOURCE_DIR}/xbmc/addons/include
                    ${CORE_SOURCE_DIR}/xbmc)

set(CMAKE_POSITION_INDEPENDENT_CODE 1)

core_add_library(KODI_guilib 1)
