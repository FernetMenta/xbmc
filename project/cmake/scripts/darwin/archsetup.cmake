set(ARCH_DEFINES -D_LINUX -DTARGET_POSIX -DTARGET_DARWIN)
set(SYSTEM_DEFINES -D_REENTRANT -D_FILE_DEFINED _FILE_OFFSET_BITS=64 _LARGEFILE64_SOURCE
                   -D__STDC_CONSTANT_MACROS -DHAVE_CONFIG_H)
set(PLATFORM_DIR darwin)
set(CMAKE_SYSTEM_NAME Darwin)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL "x86_64")
    set(ARCH x86_64-apple-darwin)
  elseif(CPU MATCHES "i386")
    set(ARCH i386-apple-darwin)
  else()
    message(WARNING "unknown CPU: ${CPU}")
  endif()
endif()

find_package(CXX11 REQUIRED)

set(ENV{PKG_CONFIG} ${NATIVEPREFIX}/bin/pkg-config)
