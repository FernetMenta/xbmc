# - Builds D3DX11Effects as external project
# Once done this will define
#
# D3DX11EFFECTS_FOUND - system has D3DX11Effects
# D3DX11EFFECTS_INCLUDE_DIRS - the D3DX11Effects include directories
# D3DCOMPILER_DLL - Path to the Direct3D Compiler

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
set(D3DX11EFFECTS_LIBRARIES ${CORE_SOURCE_DIR}/lib/win32/Effects11/libs/Effects11/${CMAKE_BUILD_TYPE}/Effects11.lib)
mark_as_advanced(D3DX11EFFECTS_FOUND)

find_file(D3DCOMPILER_DLL
          NAMES d3dcompiler_47.dll d3dcompiler_46.dll
          PATHS
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]/Redist/D3D/x86"
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]/Redist/D3D/x86"
            "$ENV{WindowsSdkDir}redist/d3d/x86"
          NO_DEFAULT_PATH)
if(NOT D3DCOMPILER_DLL)
  message(WARNING "Could NOT find Direct3D Compiler")
endif()
mark_as_advanced(D3DCOMPILER_DLL)
