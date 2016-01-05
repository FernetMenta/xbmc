set(SOURCES ConvolutionKernels.cpp)

if(WIN32)
  list(APPEND SOURCES WinVideoFilter.cpp
                      YUV2RGBShader.cpp)
endif()

if(OPENGL_FOUND OR GLES_FOUND)
  list(APPEND SOURCES GLSLOutput.cpp
                      VideoFilterShader.cpp
                      YUV2RGBShader.cpp)
endif()

core_add_library(videoshaders)
