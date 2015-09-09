# - Try to find libbluray
# Once done this will define
#
# BLURAY_FOUND - system has libbluray
# BLURAY_INCLUDE_DIRS - the libbluray include directory
# BLURAY_LIBRARIES - The libbluray libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (BLURAY libbluray>=0.7.0)
  list(APPEND BLURAY_INCLUDE_DIRS ${BLURAY_INCLUDEDIR})
else()
  find_path(BLURAY_INCLUDE_DIRS libbluray/bluray.h)
  find_library(BLURAY_LIBRARIES bluray)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bluray DEFAULT_MSG BLURAY_INCLUDE_DIRS BLURAY_LIBRARIES)

list(APPEND BLURAY_DEFINITIONS -DHAVE_LIBBLURAY=1)

mark_as_advanced(BLURAY_INCLUDE_DIRS BLURAY_LIBRARIES BLURAY_DEFINITIONS)
