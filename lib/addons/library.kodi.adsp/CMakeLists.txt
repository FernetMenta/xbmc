set(SOURCES libKODI_adsp.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CORE_SOURCE_DIR}/xbmc/addons/include
                    ${CORE_SOURCE_DIR}/xbmc)
                    #${CORE_SOURCE_DIR}/xbmc/cores/VideoPlayer/DVDDemuxers)

set(CMAKE_POSITION_INDEPENDENT_CODE 1)

core_add_library(KODI_adsp 1)
