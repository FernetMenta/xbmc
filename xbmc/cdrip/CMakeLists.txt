set(SOURCES CDDARipJob.cpp
            CDDARipper.cpp
            Encoder.cpp
            EncoderFFmpeg.cpp)

core_add_library(cdrip)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(cdrip ffmpeg)
endif()
