set(SOURCES kiss_fft.c
            kiss_fftr.c)

if(CORE_SYSTEM_NAME STREQUAL "windows")
  set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif()

core_add_library(kissfft)
