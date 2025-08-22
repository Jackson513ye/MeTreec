#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "preprocessing::preprocessing" for configuration "Release"
set_property(TARGET preprocessing::preprocessing APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(preprocessing::preprocessing PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpreprocessing.a"
  )

list(APPEND _cmake_import_check_targets preprocessing::preprocessing )
list(APPEND _cmake_import_check_files_for_preprocessing::preprocessing "${_IMPORT_PREFIX}/lib/libpreprocessing.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
