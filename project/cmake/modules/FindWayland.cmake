if(PKG_CONFIG_FOUND)
  pkg_check_modules (WAYLAND wayland-client;wayland-egl;xkbcommon)
endif()
if(WAYLAND_FOUND)
  list(APPEND WAYLAND_DEFINITIONS -DHAVE_WAYLAND=1 -DHAVE_XKBCOMMON=1)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WAYLAND DEFAULT_MSG WAYLAND_INCLUDE_DIRS WAYLAND_LIBRARIES)

mark_as_advanced(WAYLAND_INCLUDE_DIRS WAYLAND_LIBRARIES WAYLAND_DEFINITIONS)
