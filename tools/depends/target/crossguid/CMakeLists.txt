project(crossguid)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(UUID REQUIRED)
find_package(CXX11 REQUIRED)

set(SOURCES guid.cpp)

include_directories(${UUID_INCLUDE_DIRS})

add_definitions(-DGUID_LIBUUID)

add_library(crossguid ${SOURCES})
target_link_libraries(crossguid ${UUID_LIBRARIES})

install(TARGETS crossguid DESTINATION lib)
install(FILES guid.h DESTINATION include)
