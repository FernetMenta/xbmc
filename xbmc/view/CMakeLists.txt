set(SOURCES GUIViewControl.cpp
            GUIViewState.cpp
            ViewDatabase.cpp
            ViewStateSettings.cpp)

core_add_library(view)
add_dependencies(view libcpluff)
