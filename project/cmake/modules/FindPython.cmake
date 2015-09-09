# - Try to find python
# Once done this will define
#
# PYTHON_FOUND - system has PYTHON
# PYTHON_INCLUDE_DIRS - the python include directory
# PYTHON_LIBRARIES - The python libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (PYTHON python)
endif()

if(NOT PYTHON_FOUND)
  find_package(PythonLibs)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Python DEFAULT_MSG PYTHON_INCLUDE_DIRS PYTHON_LIBRARIES)

mark_as_advanced(PYTHON_INCLUDE_DIRS PYTHON_LIBRARIES)
