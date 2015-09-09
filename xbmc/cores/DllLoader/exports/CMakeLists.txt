set(SOURCES emu_dummy.cpp
            emu_kernel32.cpp
            emu_msvcrt.cpp)

core_add_library(dllexports)

add_options(C ALL_BUILDS "-fPIC")
add_library(wrapper OBJECT wrapper.c)

add_custom_target(wrapper.def ALL nm ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir/wrapper.c.o | grep __wrap | awk '{ printf(\"%s \", \$\$3) }' | sed \"s/___wrap_/__wrap_/g\" | sed \"s/__wrap_/-Wl,-wrap,/g\" > wrapper.def)

if(OS STREQUAL "Android")
  add_custom_target(patchdef ALL echo \"-L${CMAKE_INSTALL_PREFIX}/lib/dummy-libxbmc/ -lxbmc\" >> wrapper.def)
  add_dependencies(patchdef wrapper.def)
endif()

add_dependencies(wrapper.def wrapper)
