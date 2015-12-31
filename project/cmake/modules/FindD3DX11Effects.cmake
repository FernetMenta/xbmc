# - Builds D3DX11Effects as external project
# Once done this will define
#
# D3DX11EFFECTS_FOUND - system has D3DX11Effects
# D3DX11EFFECTS_INCLUDE_DIRS - the D3DX11Effects include directories
#
# and link Kodi against the D3DX11Effects libraries.

include(ExternalProject)
ExternalProject_Add(d3dx11effects
            SOURCE_DIR ${CORE_SOURCE_DIR}/lib/win32/Effects11
            PREFIX ${CORE_BUILD_DIR}/Effects11
            CONFIGURE_COMMAND ""
            BUILD_COMMAND devenv /build ${CMAKE_BUILD_TYPE}
                          ${CORE_SOURCE_DIR}/lib/win32/Effects11/Effects11_2013.sln
            INSTALL_COMMAND "")

set(D3DX11EFFECTS_FOUND 1)
set(D3DX11EFFECTS_INCLUDE_DIRS ${CORE_SOURCE_DIR}/lib/win32/Effects11/inc)
mark_as_advanced(D3DX11EFFECTS_FOUND)

# TODO: core_link_library