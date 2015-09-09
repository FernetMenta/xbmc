# - Try to find CURL
# Once done this will define
#
# CURL_FOUND - system has libcurl
# CURL_INCLUDE_DIRS - the libcurl include directory
# CURL_LIBRARIES - The libcurl libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (CURL libcurl)
  list(APPEND CURL_INCLUDE_DIRS ${CURL_INCLUDEDIR})
endif()

if(NOT CURL_FOUND)
  find_package(CURL)
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Curl DEFAULT_MSG CURL_INCLUDE_DIRS CURL_LIBRARIES)

mark_as_advanced(CURL_INCLUDE_DIRS CURL_LIBRARIES)

if(CURL_FOUND)
  include(CheckFunctionExists)
  set(CMAKE_REQUIRED_INCLUDES ${CURL_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${CURL_LIBRARIES})
  check_function_exists("CRYPTO_set_locking_callback" HAS_CURL_STATIC)
endif()
if(HAS_CURL_STATIC)
  list(APPEND CURL_DEFINITIONS -DHAS_CURL_STATIC=1)
endif()
