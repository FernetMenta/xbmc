set(SOURCES EventLoop.cpp
            InputFactory.cpp
            Keyboard.cpp
            KeyboardProcessor.cpp
            Pointer.cpp
            PointerProcessor.cpp
            PollThread.cpp
            Seat.cpp
            Wayland11EventQueueStrategy.cpp
            Wayland12EventQueueStrategy.cpp)

core_add_library(windowing_wayland)
