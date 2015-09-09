# - Try to find shairport
# Once done this will define
#
# SHAIRPORT_FOUND - system has libshairport
# SHAIRPORT_INCLUDE_DIRS - the libshairport include directory
# SHAIRPORT_LIBRARIES - The libshairport libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (SHAIRPORT libshairport)
  list(APPEND SHAIRPORT_INCLUDE_DIRS ${SHAIRPORT_INCLUDEDIR})
else()
  find_path(SHAIRPORT_INCLUDE_DIRS shairport/shairport.h)
  find_library(SHAIRPORT_LIBRARIES shairport)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Shairport DEFAULT_MSG SHAIRPORT_INCLUDE_DIRS SHAIRPORT_LIBRARIES)

list(APPEND SHAIRPORT_DEFINITIONS -DHAVE_LIBSHAIRPORT=1)

mark_as_advanced(SHAIRPORT_INCLUDE_DIRS SHAIRPORT_LIBRARIES SHAIRPORT_DEFINITIONS)
