set(SOURCES Callback.cpp
            Compositor.cpp
            Display.cpp
            OpenGLSurface.cpp
            Output.cpp
            Region.cpp
            Registry.cpp
            Shell.cpp
            ShellSurface.cpp
            Surface.cpp
            WaylandLibraries.cpp
            XBMCConnection.cpp
            XBMCSurface.cpp)

core_add_library(windowing_egl_wayland)
