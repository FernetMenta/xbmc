function(core_link_library lib wraplib)
  set(export -bundle -undefined dynamic_lookup -read_only_relocs suppress
             -Wl,-alias_list,${CORE_BUILD_DIR}/cores/dll-loader/exports/wrapper.def
             ${CORE_BUILD_DIR}/cores/dll-loader/exports/CMakeFiles/wrapper.dir/wrapper.c.o)
  set(check_arg "")
  if(TARGET ${lib})
    set(target ${lib})
    set(link_lib ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${lib}/${lib}.a)
    set(check_arg ${ARGV2})
    set(data_arg  ${ARGV3})
  else()
    set(target ${ARGV2})
    set(link_lib ${lib})
    set(check_arg ${ARGV3})
    set(data_arg ${ARGV4})
  endif()
  if(check_arg STREQUAL "export")
    set(export ${export}
        -Wl,--version-script=${ARGV3})
  elseif(check_arg STREQUAL "nowrap")
    set(export ${data_arg})
  elseif(check_arg STREQUAL "extras")
    foreach(arg ${data_arg})
      list(APPEND export ${arg})
    endforeach()
  endif()
  get_filename_component(dir ${wraplib} PATH)

  # We can't simply pass the linker flags to the args section of the custom command
  # because cmake will add quotes around it (and the linker will fail due to those).
  # We need to do this handstand first ...
  separate_arguments(CUSTOM_COMMAND_ARGS_LDFLAGS UNIX_COMMAND "${CMAKE_SHARED_LINKER_FLAGS}")

  add_custom_command(OUTPUT ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
                     COMMAND ${CMAKE_C_COMPILER}
                     ARGS    ${CUSTOM_COMMAND_ARGS_LDFLAGS} ${export} -Wl,-force_load ${link_lib}
                             -o ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                     DEPENDS ${target} wrapper.def wrapper
                     VERBATIM)

  # Uncomment to create wrap_<lib> targets for debugging
  #get_filename_component(libname ${wraplib} NAME_WE)
  #add_custom_target(wrap_${libname} ALL DEPENDS ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX})

  list(APPEND WRAP_FILES ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX})
  set(WRAP_FILES ${WRAP_FILES} PARENT_SCOPE)
endfunction()

function(find_soname lib)
  if(ARGV1)
    set(liblow ${ARGV1})
  else()
    string(TOLOWER ${lib} liblow)
  endif()
  if(${lib}_LDFLAGS)
    set(link_lib "${${lib}_LDFLAGS}")
  else()
    if(IS_ABSOLUTE "${${lib}_LIBRARIES}")
      set(link_lib "${${lib}_LIBRARIES}")
    else()
      set(link_lib -l${${lib}_LIBRARIES})
    endif()
  endif()
  execute_process(COMMAND ${CMAKE_C_COMPILER} -nostdlib -o /dev/null -Wl,-M ${link_lib} 
                  COMMAND grep LOAD.*${liblow}
                  ERROR_QUIET
                  OUTPUT_VARIABLE ${lib}_FILENAME)
  string(REPLACE "LOAD " "" ${lib}_FILENAME "${${lib}_FILENAME}")
  string(STRIP "${${lib}_FILENAME}" ${lib}_FILENAME)
  if(${lib}_FILENAME)
    execute_process(COMMAND objdump -p ${${lib}_FILENAME}
                    COMMAND grep SONAME.*${liblow}
                    ERROR_QUIET
                    OUTPUT_VARIABLE ${lib}_SONAME)
    string(REPLACE "SONAME " "" ${lib}_SONAME ${${lib}_SONAME})
    string(STRIP ${${lib}_SONAME} ${lib}_SONAME)
    message(STATUS "${lib} soname: ${${lib}_SONAME}")
    set(${lib}_SONAME ${${lib}_SONAME} PARENT_SCOPE)
  endif()
endfunction()
