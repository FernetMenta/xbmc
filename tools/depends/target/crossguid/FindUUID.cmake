# - Try to find UUID
# Once done this will define
#
# UUID_FOUND - system has libuuid
# UUID_INCLUDE_DIRS - the libuuid include directory
# UUID_LIBRARIES - The libuuid libraries

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (UUID uuid)
else()
  find_path(UUID_INCLUDE_DIRS uuid/uuid.h)
  find_library(UUID_LIBRARIES uuid)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UUID DEFAULT_MSG UUID_INCLUDE_DIRS UUID_LIBRARIES)

mark_as_advanced(UUID_INCLUDE_DIRS UUID_LIBRARIES)
