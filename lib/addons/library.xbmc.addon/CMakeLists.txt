set(SOURCES libXBMC_addon.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CORE_SOURCE_DIR}/xbmc/addons/include
                    ${CORE_SOURCE_DIR}/xbmc)

add_options(CXX ALL_BUILDS "-fPIC")

core_add_library(XBMC_addon NO_MAIN_DEPENDS)
