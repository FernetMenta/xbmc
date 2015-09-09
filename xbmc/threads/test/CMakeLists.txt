set(SOURCES TestEvent.cpp
            TestSharedSection.cpp
            TestAtomics.cpp
            TestThreadLocal.cpp)

include_directories(${CORE_SOURCE_DIR}/lib/gtest/include)

core_add_test_library(threads_test)
