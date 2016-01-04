#.rst:
# FindPCRE
# --------
# Finds the PCRECPP library
#
# This will will define the following variables::
#
# PCRE_FOUND - system has libpcrecpp
# PCRE_INCLUDE_DIRS - the libpcrecpp include directory
# PCRE_LIBRARIES - The libpcrecpp libraries
#
# and the following imported targets::
#
#   Foo::Foo   - The Foo library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PCRE libpcrecpp)
endif()

find_path(PCRE_INCLUDE_DIR pcrecpp.h
                           PATHS ${PC_PCRE_INCLUDEDIR})
find_library(PCRE_LIBRARY_RELEASE NAMES pcrecpp
                                  PATHS ${PC_PCRE_LIBDIR})
find_library(PCRE_LIBRARY_DEBUG NAMES pcrecppd
                                PATHS ${PC_PCRE_LIBDIR})
set(PCRE_VERSION ${PC_PCRE_VERSION})

include(SelectLibraryConfigurations)
select_library_configurations(PCRE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE
                                  REQUIRED_VARS PCRE_LIBRARY PCRE_INCLUDE_DIR
                                  VERSION_VAR PCRE_VERSION)

if(PCRE_FOUND)
  set(PCRE_LIBRARIES ${PCRE_LIBRARY})
  set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})

  if(NOT TARGET PCRE::PCRE)
    add_library(PCRE::PCRE UNKNOWN IMPORTED)
    if(PCRE_LIBRARY_RELEASE)
      set_target_properties(PCRE::PCRE PROPERTIES
                                       IMPORTED_CONFIGURATIONS RELEASE
                                       IMPORTED_LOCATION "${PCRE_LIBRARY_RELEASE}")
    endif()
    if(PCRE_LIBRARY_DEBUG)
      set_target_properties(PCRE::PCRE PROPERTIES
                                       IMPORTED_CONFIGURATIONS DEBUG
                                       IMPORTED_LOCATION "${PCRE_LIBRARY_DEBUG}")
    endif()
    set_target_properties(PCRE::PCRE PROPERTIES
                                     INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(PCRE_INCLUDE_DIR PCRE_LIBRARY)
