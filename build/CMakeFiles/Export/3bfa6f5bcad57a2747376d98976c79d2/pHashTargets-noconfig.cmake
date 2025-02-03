#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pHash" for configuration ""
set_property(TARGET pHash APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(pHash PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libpHash.a"
  )

list(APPEND _cmake_import_check_targets pHash )
list(APPEND _cmake_import_check_files_for_pHash "${_IMPORT_PREFIX}/lib/libpHash.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
