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

if(VTB_FOUND)
  list(APPEND SOURCES RendererVTB.cpp)
endif()

if(IMX_FOUND)
  list(APPEND SOURCES RendererIMX.cpp)
endif()

if(OMX_FOUND)
  list(APPEND SOURCES RendererOMX.cpp)
endif()

if (STAGEFRIGHT_FOUND)
  list(APPEND SOURCES RendererStagefright.cpp)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCE MMALRenderer.cpp)
endif()

core_add_library(videorenderers_hwdec)
add_dependencies(videorenderers_hwdec ffmpeg)
