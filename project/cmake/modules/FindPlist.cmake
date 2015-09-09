# - Try to find PLIST
# Once done this will define
#
# PLIST_FOUND - system has libplist
# PLIST_INCLUDE_DIRS - the libplist include directory
# PLIST_LIBRARIES - The libplist libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (PLIST libplist)
else()
  find_path(PLIST_INCLUDE_DIRS plist/plist.h)
  find_library(PLIST_LIBRARIES plist)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Plist DEFAULT_MSG PLIST_INCLUDE_DIRS PLIST_LIBRARIES)

list(APPEND PLIST_DEFINITIONS -DHAVE_LIBPLIST=1)

mark_as_advanced(PLIST_INCLUDE_DIRS PLIST_LIBRARIES PLIST_DEFINITIONS)
