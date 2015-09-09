# - Try to find shairplay
# Once done this will define
#
# SHAIRPLAY_FOUND - system has libshairplay
# SHAIRPLAY_INCLUDE_DIRS - the libshairplay include directory
# SHAIRPLAY_LIBRARIES - The libshairplay libraries

find_path(SHAIRPLAY_INCLUDE_DIRS shairplay/raop.h)
find_library(SHAIRPLAY_LIBRARIES shairplay)

if(SHAIRPLAY_INCLUDE_DIRS AND SHAIRPLAY_LIBRARIES)
  include(CheckCSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES ${SHAIRPLAY_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${SHAIRPLAY_LIBRARIES})
  check_c_source_compiles("#include <shairplay/raop.h>

                           int main()
                           {
                             struct raop_callbacks_s foo;
                             foo.cls;
                             return 0;
                           }
                          " HAVE_SHAIRPLAY_CALLBACK_CLS)
  if (HAVE_SHAIRPLAY_CALLBACK_CLS)
    set(SHAIRPLAY_FOUND 1 PARENT_SCOPE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Shairplay DEFAULT_MSG SHAIRPLAY_INCLUDE_DIRS SHAIRPLAY_LIBRARIES)

list(APPEND SHAIRPLAY_DEFINITIONS -DHAVE_LIBSHAIRPLAY=1)

mark_as_advanced(SHAIRPLAY_FOUND SHAIRPLAY_INCLUDE_DIRS SHAIRPLAY_LIBRARIES SHAIRPLAY_DEFINITIONS)
