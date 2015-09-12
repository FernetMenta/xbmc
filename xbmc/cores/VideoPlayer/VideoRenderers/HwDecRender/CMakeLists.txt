set(SOURCES)

if(DXVA_FOUND)
  list(APPEND SOURCES DXVAHD.cpp)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES RendererVAAPI.cpp)
endif()

if(VDPAU_FOUND)
  list(APPEND SOURCES RendererVDPAU.cpp)
endif()

if(VDA_FOUND)
  list(APPEND SOURCES RendererVDA.cpp)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES MMALRenderer.cpp)
endif()

core_add_library(videorenderers_hwdec)
add_dependencies(videorenderers_hwdec ffmpeg)
