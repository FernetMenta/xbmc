set(SOURCES Database.cpp
            DatabaseQuery.cpp
            dataset.cpp
            qry_dat.cpp
            sqlitedataset.cpp)

if(MYSQLCLIENT_FOUND)
  list(APPEND SOURCES mysqldataset.cpp)
endif()

core_add_library(dbwrappers)
