set(SOURCES WinSystemEGL.cpp
            EGLWrapper.cpp)

if(WAYLAND_FOUND)
  list(APPEND SOURCES EGLNativeTypeWayland.cpp)
endif()

core_add_library(windowing_egl)
