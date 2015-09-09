set(SOURCES EpgContainer.cpp
            Epg.cpp
            EpgDatabase.cpp
            EpgInfoTag.cpp
            EpgSearchFilter.cpp
            GUIEPGGridContainer.cpp)

core_add_library(epg)
add_dependencies(epg libcpluff)
