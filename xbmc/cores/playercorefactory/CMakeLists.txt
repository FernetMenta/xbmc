set(SOURCES PlayerCoreFactory.cpp
            PlayerSelectionRule.cpp)

core_add_library(playercorefactory)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(playercorefactory ffmpeg)
endif()
