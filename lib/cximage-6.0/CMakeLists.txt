set(SOURCES jbig/jbig.c
            jbig/jbig_tab.c
            raw/libdcr.c
            CxImage/DllInterface.cpp
            CxImage/tif_xfile.cpp
            CxImage/ximabmp.cpp
            CxImage/ximadsp.cpp
            CxImage/ximaenc.cpp
            CxImage/ximaexif.cpp
            CxImage/ximage.cpp
            CxImage/ximagif.cpp
            CxImage/ximahist.cpp
            CxImage/ximaico.cpp
            CxImage/ximainfo.cpp
            CxImage/ximaint.cpp
            CxImage/ximajas.cpp
            CxImage/ximajbg.cpp
            CxImage/ximajpg.cpp
            CxImage/ximalpha.cpp
            CxImage/ximalyr.cpp
            CxImage/ximamng.cpp
            CxImage/ximapal.cpp
            CxImage/ximapcx.cpp
            CxImage/ximapng.cpp
            CxImage/ximaraw.cpp
            CxImage/ximasel.cpp
            CxImage/ximaska.cpp
            CxImage/ximatga.cpp
            CxImage/ximath.cpp
            CxImage/ximatif.cpp
            CxImage/ximatran.cpp
            CxImage/ximawbmp.cpp
            CxImage/ximawmf.cpp
            CxImage/ximawnd.cpp
            CxImage/xmemfile.cpp)

add_definitions(-DXBMC -D_DLL)

include_directories(${ZLIB_INCLUDE_DIRS}
                    ${JPEG_INCLUDE_DIRS}
                    ${TIFF_INCLUDE_DIRS}
                    ${PNG_INCLUDE_DIRS}
                    ${JASPER_INCLUDE_DIRS})


add_options(ALL_LANGUAGES ALL_BUILDS "-fPIC")

core_add_library(cximage 1)
