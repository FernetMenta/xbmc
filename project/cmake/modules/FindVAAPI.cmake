# -*- cmake -*-

# - Find VA-API
# Find the VA-API includes and library
# This module defines
#  VAAPI_INCLUDE_DIRS, where to find db.h, etc.
#  VAAPI_LIBRARIES, the libraries needed to use VA-API.
#  VAAPI_FOUND, If false, do not try to use VA-API.
# also defined, but not for general use are
#  VAAPI_LIBRARY, where to find the VA-API library.

find_path(VAAPI_INCLUDE_DIRS va/va.h)

find_library(VA_LIBRARY
             NAMES va)

find_library(VAX11_LIBRARY
             NAMES va-x11)

find_library(VAGLX_LIBRARY
             NAMES va-glx)

set(VAAPI_LIBRARIES ${VAGLX_LIBRARY} ${VAX11_LIBRARY} ${VA_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VAAPI DEFAULT_MSG VAAPI_LIBRARIES VAAPI_INCLUDE_DIRS)

list(APPEND VAAPI_DEFINITIONS -DHAVE_LIBVA=1)

mark_as_advanced(VAAPI_INCLUDE_DIRS VAAPI_LIBRARIES VAAPI_DEFINITIONS)
