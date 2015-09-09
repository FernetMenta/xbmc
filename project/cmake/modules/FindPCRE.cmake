# - Try to find pcrecpp
# Once done this will define
#
# PCRE_FOUND - system has libpcrecpp
# PCRE_INCLUDE_DIRS - the libpcrecpp include directory
# PCRE_LIBRARIES - The libpcrecpp libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (PCRE libpcrecpp)
  if(NOT PCRE_INCLUDE_DIRS)
    set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDEDIR})
  endif()
else()
  find_path(PCRE_INCLUDE_DIRS pcrecpp.h)
  find_library(PCRE_LIBRARIES pcrecpp)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_INCLUDE_DIRS PCRE_LIBRARIES)

mark_as_advanced(PCRE_INCLUDE_DIRS PCRE_LIBRARIES)
