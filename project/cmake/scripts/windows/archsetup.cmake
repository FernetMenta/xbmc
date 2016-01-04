set(ARCH_DEFINES -D_WINDOWS -DTARGET_WINDOWS)
set(SYSTEM_DEFINES -DNOMINMAX -D_USE_32BIT_TIME_T -DHAS_DX -D__STDC_CONSTANT_MACROS
                   -DTAGLIB_STATIC -DNPT_CONFIG_ENABLE_LOGGING
                   -DPLT_HTTP_DEFAULT_USER_AGENT="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DPLT_HTTP_DEFAULT_SERVER="UPnP/1.0 DLNADOC/1.50 Kodi")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND SYSTEM_DEFINES -DD3D_DEBUG_INFO -D_SECURE_SCL=0 -D_HAS_ITERATOR_DEBUGGING=0)
endif()
set(PLATFORM_DIR win32)
add_options(CXX ALL_BUILDS "/wd\"4996\"")

# Precompiled headers fail with per target output directory. (needs CMake 3.1)
set(PRECOMPILEDHEADER_DIR ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/objs)
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR})

set(CMAKE_SYSTEM_NAME Windows)
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../BuildDependencies)
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../../lib/win32)
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../BuildDependencies/lib/Release-vc120
                                      ${PROJECT_SOURCE_DIR}/../BuildDependencies/lib/Debug-vc120)
set(JPEG_NAMES ${JPEG_NAMES} jpeg-static)
set(PYTHON_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/../BuildDependencies/include/python)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL "AMD64")
    set(ARCH x86_64-windows)
  elseif(CPU MATCHES "i.86")
    set(ARCH i486-windows)
  else()
    message(WARNING "unknown CPU: ${CPU}")
  endif()
endif()

# For #pragma comment(lib X)
# TODO: It would certainly be better to handle these libraries via CMake modules.
link_directories(${PROJECT_SOURCE_DIR}/../../lib/win32/ffmpeg/.libs
                 ${PROJECT_SOURCE_DIR}/../BuildDependencies/lib
                 ${PROJECT_SOURCE_DIR}/../BuildDependencies/lib/Release-vc120
                 ${PROJECT_SOURCE_DIR}/../BuildDependencies/lib/Debug-vc120)

# Compile with /MT (to be compatible with the dependent libraries)
foreach(CompilerFlag CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

set(_nodefaultlibs_RELEASE libc msvcrt libci msvcprt)
set(_nodefaultlibs_DEBUG libc msvcrt libcmt libcpmt msvcrtd msvcprtd)
foreach(_lib ${NODEFAULTLIBS_RELEASE})
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:\"${_lib}\"")
endforeach()
foreach(_lib ${NODEFAULTLIBS_DEBUG})
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:\"${_lib}\"")
endforeach()
