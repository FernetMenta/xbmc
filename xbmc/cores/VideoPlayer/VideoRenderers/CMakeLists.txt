set(SOURCES BaseRenderer.cpp
            OverlayRenderer.cpp
            OverlayRendererGUI.cpp
            OverlayRendererUtil.cpp
            RenderCapture.cpp
            RenderFlags.cpp
            RenderManager.cpp)

if(WIN32)
  list(APPEND SOURCES WinRenderer.cpp
                      OverlayRendererDX.cpp)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES OverlayRendererGL.cpp
                      LinuxRendererGL.cpp)
endif()

if(GLES_FOUND)
  list(APPEND SOURCES LinuxRendererGLES.cpp
                      OverlayRendererGL.cpp)

endif()

core_add_library(videorenderers)
add_dependencies(videorenderers ffmpeg)
