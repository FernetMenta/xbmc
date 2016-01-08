function(core_link_library lib wraplib)
#  set(export -Wl,--unresolved-symbols=ignore-all
#             `cat ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/wrapper.def`
#             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/CMakeFiles/wrapper.dir/wrapper.c.o)
#  set(check_arg "")
#  if(TARGET ${lib})
#    set(target ${lib})
#    set(link_lib ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${lib}/${lib}.a)
#    set(check_arg ${ARGV2})
#    set(data_arg  ${ARGV3})
#  else()
#    set(target ${ARGV2})
#    set(link_lib ${lib})
#    set(check_arg ${ARGV3})
#    set(data_arg ${ARGV4})
#  endif()
#  if(check_arg STREQUAL "export")
#    set(export ${export}
#        -Wl,--version-script=${ARGV3})
#  elseif(check_arg STREQUAL "nowrap")
#    set(export ${data_arg})
#  elseif(check_arg STREQUAL "extras")
#    foreach(arg ${data_arg})
#      list(APPEND export ${arg})
#    endforeach()
#  endif()
#  get_filename_component(dir ${wraplib} PATH)
#  add_custom_command(OUTPUT ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
#                     COMMAND cmake -E make_directory ${dir}
#                     COMMAND ${CMAKE_C_COMPILER}
#                     ARGS    -Wl,--whole-archive
#                             ${link_lib}
#                             -Wl,--no-whole-archive -lm
#                             -shared -o ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
#                             ${export}
#                     DEPENDS ${target} wrapper.def wrapper)
#  list(APPEND WRAP_FILES ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX})
#  set(WRAP_FILES ${WRAP_FILES} PARENT_SCOPE)
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

# Add precompiled header to target
# Arguments:
#   target existing target that will be set up to compile with a precompiled header
#   pch_header the precompiled header file
#   pch_source the precompiled header source file
# Optional Arguments:
#   PCH_TARGET build precompiled header as separate target with the given name
#              so that the same precompiled header can be used for multiple libraries
#   EXCLUDE_SOURCES if not all target sources shall use the precompiled header,
#                   the relevant files can be listed here
# On return:
#   Compiles the pch_source into a precompiled header and adds the header to
#   the given target
function(add_precompiled_header target pch_header pch_source)
  cmake_parse_arguments(PCH "" "PCH_TARGET" "EXCLUDE_SOURCES" ${ARGN})

  if(PCH_PCH_TARGET)
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${PCH_PCH_TARGET}.pch)
  else()
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${target}.pch)
  endif()

  # Set compile options and dependency for sources
  get_target_property(sources ${target} SOURCES)
  list(REMOVE_ITEM sources ${pch_source})
  foreach(exclude_source IN LISTS PCH_EXCLUDE_SOURCES)
    list(REMOVE_ITEM sources ${exclude_source})
  endforeach()
  set_source_files_properties(${sources}
                              PROPERTIES COMPILE_FLAGS "/Yu\"${pch_header}\" /Fp\"${pch_binary}\" /FI\"${pch_header}\""
                              OBJECT_DEPENDS "${pch_binary}")

  # Set compile options for precompiled header
  if(NOT PCH_PCH_TARGET OR NOT TARGET ${PCH_PCH_TARGET}_pch)
    set_source_files_properties(${pch_source}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${pch_header}\" /Fp\"${pch_binary}\""
                                OBJECT_OUTPUTS "${pch_binary}")
  endif()

  # Compile precompiled header
  if(PCH_PCH_TARGET)
    # As own target for usage in multiple libraries
    if(NOT TARGET ${PCH_PCH_TARGET}_pch)
      add_library(${PCH_PCH_TARGET}_pch STATIC ${pch_source})
      set_target_properties(${PCH_PCH_TARGET}_pch PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR})
    endif()
    # From VS2012 onwards, precompiled headers have to be linked against (LNK2011).
    target_link_libraries(${target} PUBLIC ${PCH_PCH_TARGET}_pch)
    set_target_properties(${target} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR})
  else()
    # As part of the target
    target_sources(${target} PRIVATE ${pch_source})
  endif()
endfunction()

# Adds an FX-compiled shader to a target
#   Creates a custom command that FX-compiles the given shader and adds the
#   generated header file to the given target.
# Arguments:
#   target Target to add the FX-compiled shader to
#   hlsl HLSL shader input file
#   profile HLSL profile that specifies the shader model
#   entrypoint Shader entry point
# On return:
#   FXC_FILE is set to the name of the generated header file.
find_program(FXC fxc)
function(add_shader_dx target hlsl profile entrypoint)
  get_filename_component(file ${hlsl} NAME_WE)
  add_custom_command(OUTPUT ${file}.h
                     COMMAND ${FXC} /Fh ${file}.h
                                    /E ${entrypoint}
                                    /T ${profile}
                                    /Vn ${file}
                                    /Qstrip_reflect
                                    ${hlsl}
                     DEPENDS ${hlsl}
                     COMMENT "FX compile ${hlsl}"
                     VERBATIM)
  target_sources(${target} PRIVATE ${file}.h)
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
