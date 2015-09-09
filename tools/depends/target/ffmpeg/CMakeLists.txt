project(ffmpeg)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(GnuTls)

set(ffmpeg_conf "")
#if(CMAKE_BUILD_TYPE STREQUAL "Release")
  #  list(APPEND ffmpeg_conf --enable-optimizations)
  #else()
  #  list(APPEND ffmpeg_conf --disable-optimizations)
  #endif()

if(GNUTLS_FOUND)
  list(APPEND ffmpeg_conf --enable-gnutls)
endif()

include(ExternalProject)
externalproject_add(ffmpeg
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --disable-muxers
                      --enable-zlib
                      --enable-bzlib
                      --enable-muxer=spdif
                      --enable-muxer=adts
                      --enable-muxer=asf
                      --enable-muxer=ipod
                      --disable-encoders
                      --enable-encoder=ac3
                      --enable-encoder=aac
                      --enable-encoder=wmav2
                      --disable-decoder=mpeg_xvmc
                      --disable-devices
                      --disable-ffprobe
                      --disable-ffplay
                      --disable-ffserver
                      --disable-ffmpeg
                      --disable-crystalhd
                      --enable-static
                      --disable-shared
                      --disable-doc
                      --enable-postproc
                      --enable-gpl
                      --enable-vdpau
                      --enable-vaapi
                      --enable-protocol=http
                      --enable-pthreads
                      --enable-runtime-cpudetect
                      --enable-pic
                      --cc=${CMAKE_C_COMPILER}
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      ${ffmpeg_conf}
                    BUILD_COMMAND make)

install(CODE "Message(Done)")

# Quell warnings
set(BUILD_SHARED_LIBS)
set(XBMC_BUILD_DIR)
set(KODI_BUILD_DIR)
