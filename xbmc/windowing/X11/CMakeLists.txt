set(SOURCES GLContextEGL.cpp
            GLContextGLX.cpp
            WinSystemX11.cpp
            WinSystemX11GLContext.cpp
            XRandR.cpp)

core_add_library(windowing_X11)
