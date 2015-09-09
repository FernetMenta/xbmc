# - Try to find libtag
# Once done this will define
#
# TAGLIB_FOUND - system has libtag
# TAGLIB_INCLUDE_DIRS - the libtag include directory
# TAGLIB_LIBRARIES - The libtag libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (TAGLIB taglib>=1.8.0)
endif()

if(NOT TAGLIB_FOUND)
  find_path(TAGLIB_INCLUDE_DIRS taglib/tag.h)
  find_library(TAGLIB_LIBRARIES tag)
endif()

# Workaround broken .pc file
list(APPEND TAGLIB_LIBRARIES ${ZLIB_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TagLib REQUIRED_VARS TAGLIB_INCLUDE_DIRS TAGLIB_LIBRARIES)

mark_as_advanced(TAGLIB_INCLUDE_DIRS TAGLIB_LIBRARIES)
