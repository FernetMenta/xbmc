set(SOURCES CDDARipJob.cpp
            CDDARipper.cpp
            Encoder.cpp
            EncoderFFmpeg.cpp)

core_add_library(cdrip)
add_dependencies(cdrip ffmpeg)
