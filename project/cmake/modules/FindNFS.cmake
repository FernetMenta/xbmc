# - Try to find libnfs
# Once done this will define
#
# NFS_FOUND - system has libnfs
# NFS_INCLUDE_DIRS - the libnfs include directory
# NFS_LIBRARIES - The libnfs libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (NFS libnfs)
  list(APPEND NFS_INCLUDE_DIRS ${NFS_INCLUDEDIR})
else()
  find_path(NFS_INCLUDE_DIRS nfsc/libnfs.h)
  find_library(NFS_LIBRARIES nfs)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NFS DEFAULT_MSG NFS_INCLUDE_DIRS NFS_LIBRARIES)

list(APPEND NFS_DEFINITIONS -DHAVE_LIBNFS=1)

mark_as_advanced(NFS_INCLUDE_DIRS NFS_LIBRARIES NFS_DEFINITIONS)
