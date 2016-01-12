set(SOURCES AEFactory.cpp
            AEResampleFactory.cpp
            AESinkFactory.cpp
            DSPAddons/ActiveAEDSPAddon.cpp
            DSPAddons/ActiveAEDSPDatabase.cpp
            DSPAddons/ActiveAEDSPProcess.cpp
            DSPAddons/ActiveAEDSP.cpp
            DSPAddons/ActiveAEDSPMode.cpp
            Utils/AEChannelInfo.cpp
            Utils/AEBuffer.cpp
            Utils/AEStreamInfo.cpp
            Utils/AEUtil.cpp
            Utils/AEPackIEC61937.cpp
            Utils/AEBitstreamPacker.cpp
            Utils/AEDeviceInfo.cpp
            Utils/AELimiter.cpp
            Encoders/AEEncoderFFmpeg.cpp
            Sinks/AESinkNULL.cpp
            Engines/ActiveAE/ActiveAE.cpp
            Engines/ActiveAE/ActiveAESink.cpp
            Engines/ActiveAE/ActiveAEStream.cpp
            Engines/ActiveAE/ActiveAESound.cpp
            Engines/ActiveAE/ActiveAEBuffer.cpp)

if(ALSA_FOUND)
  list(APPEND SOURCES Sinks/AESinkALSA.cpp
                      Sinks/alsa/ALSADeviceMonitor.cpp
                      Sinks/alsa/ALSAHControlMonitor.cpp
                      Sinks/AESinkOSS.cpp
                      Utils/AEELDParser.cpp)
endif()

if(PULSEAUDIO_FOUND)
  list(APPEND SOURCES Sinks/AESinkPULSE.cpp)
endif()

if(FFMPEG_FOUND)
  list(APPEND SOURCES Engines/ActiveAE/ActiveAEResampleFFMPEG.cpp)
endif()

if(WIN32)
  list(APPEND SOURCES Sinks/AESinkDirectSound.cpp
                      Sinks/AESinkWASAPI.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "rbpi")
  list(APPEND SOURCES Engines/ActiveAE/ActiveAEResamplePi.cpp
                      Sinks/AESinkPi.cpp)
endif()

if(CORE_SYSTEM_NAME STREQUAL "darwin" OR CORE_SYSTEM_NAME STREQUAL "ios")
  list(APPEND SOURCES Sinks/osx/AEDeviceEnumerationOSX.cpp
                      Sinks/osx/CoreAudioChannelLayout.cpp
                      Sinks/osx/CoreAudioDevice.cpp
                      Sinks/osx/CoreAudioHardware.cpp
                      Sinks/osx/CoreAudioHelpers.cpp
                      Sinks/osx/CoreAudioStream.cpp)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

core_add_library(audioengine)
add_dependencies(audioengine libcpluff ffmpeg)
