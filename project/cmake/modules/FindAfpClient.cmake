# - Try to find afpclient
# Once done this will define
#
# AFPCLIENT_FOUND - system has libafpclient
# AFPCLIENT_INCLUDE_DIRS - the libafpclient include directory
# AFPCLIENT_LIBRARIES - The libafpclient libraries

find_path(AFPCLIENT_INCLUDE_DIRS afpfs-ng/libafpclient.h)
find_library(AFPCLIENT_LIBRARIES afpclient)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AfpClient DEFAULT_MSG AFPCLIENT_INCLUDE_DIRS AFPCLIENT_LIBRARIES)

list(APPEND AFPCLIENT_DEFINITIONS -DHAVE_LIBAFPCLIENT=1)

mark_as_advanced(AFPCLIENT_INCLUDE_DIRS AFPCLIENT_LIBRARIES AFPCLIENT_DEFINITIONS)
