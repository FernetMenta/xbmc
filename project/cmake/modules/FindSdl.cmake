# - Try to find SDL2
# Once done this will define
#
# SDL_FOUND - system has libSDL
# SDL_INCLUDE_DIRS - the libSDL include directory
# SDL_LIBRARIES - The libSDL libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (SDL sdl2)
else()
  find_package(SDL)
  list(APPEND SDL_INCLUDE_DIRS ${SDL_INCLUDE_DIR})
  list(APPEND SDL_LIBRARIES ${SDL_LIBRARIES})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sdl DEFAULT_MSG SDL_INCLUDE_DIRS SDL_LIBRARIES)

list(APPEND SDL_DEFINITIONS -DHAVE_SDL=1)

mark_as_advanced(SDL_INCLUDE_DIRS SDL_LIBRARIES SDL_DEFINITIONS)
