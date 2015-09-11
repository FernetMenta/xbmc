set(SOURCES src/md5.cpp
            src/XBTFWriter.cpp
            src/TexturePacker.cpp
            src/DecoderManager.cpp
            src/decoder/JPGDecoder.cpp
            src/decoder/PNGDecoder.cpp
            src/decoder/GifHelper.cpp
            src/decoder/GIFDecoder.cpp
            ${CORE_SOURCE_DIR}/xbmc/guilib/XBTF.cpp)

set(CMAKE_POSITITION_INDEPENDENT_CODE 1)

# Fixme
add_definitions(-DTARGET_POSIX -DUSE_LZO_PACKING)

list(APPEND CMAKE_MODULE_PATH ${CORE_SOURCE_DIR}/project/cmake/modules)

find_package(Lzo2 REQUIRED)
find_package(PNG REQUIRED)
find_package(GIF REQUIRED)
find_package(JPEG REQUIRED)

if(GIF_VERSION LESS 4)
  message(FATAL_ERROR "giflib < 4 not supported")
else()
  file(STRINGS ${GIF_INCLUDE_DIR}/gif_lib.h GIFSTRINGS)
  string(REGEX MATCH "GIFLIB_MAJOR ([0-9])" GIFLIB_MAJOR ${GIFSTRINGS})
  if(GIFLIB_MAJOR)
    separate_arguments(GIFLIB_MAJOR)
    list(GET GIFLIB_MAJOR 1 GIFLIB_MAJOR)
  else()
    set(GIFLIB_MAJOR ${GIF_VERSION})
  endif()
  if(NOT GIFLIB_MAJOR OR GIFLIB_MAJOR LESS 5)
    message(WARNING "giflib${GIFLIB_MAJOR} support is experimental. Consider updating to giflib5")
  endif()
endif()

include_directories(${PNG_INCLUDE_DIRS}
                    ${JPEG_INCLUDE_DIR}
                    ${GIF_INCLUDE_DIR}
                    ${CORE_SOURCE_DIR}/xbmc
                    ${CORE_SOURCE_DIR}/lib
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder)
add_executable(TexturePacker ${SOURCES})
target_link_libraries(TexturePacker ${SYSTEM_LDFLAGS} squish
                                    ${GIF_LIBRARIES}
                                    ${PNG_LIBRARIES}
                                    ${JPEG_LIBRARIES}
                                    ${LZO2_LIBRARIES})
