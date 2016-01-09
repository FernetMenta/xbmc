set(SOURCES libXBMC_codec.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CORE_SOURCE_DIR}/addons/library.xbmc.addon
                    ${CORE_SOURCE_DIR}/xbmc/addons/include)
add_options(CXX ALL_BUILDS "-fPIC")

core_add_library(XBMC_codec NO_MAIN_DEPENDS)
