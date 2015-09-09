set(SOURCES PlayerCoreFactory.cpp
            PlayerSelectionRule.cpp)

core_add_library(playercorefactory)
add_dependencies(playercorefactory ffmpeg)
