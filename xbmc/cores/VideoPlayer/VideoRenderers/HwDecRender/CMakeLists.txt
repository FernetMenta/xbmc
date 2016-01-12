set(SOURCES)

if(CORE_SYSTEM_NAME STREQUAL "windows")
  list(APPEND SOURCES DXVAHD.cpp)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES RendererVAAPI.cpp)
endif()

if(VDPAU_FOUND)
  list(APPEND SOURCES RendererVDPAU.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "darwin")
  list(APPEND SOURCES RendererVDA.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "ios")
  list(APPEND SOURCES RendererVTB.cpp)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES MMALRenderer.cpp)
endif()

core_add_library(videorenderers_hwdec)
add_dependencies(videorenderers_hwdec ffmpeg)
