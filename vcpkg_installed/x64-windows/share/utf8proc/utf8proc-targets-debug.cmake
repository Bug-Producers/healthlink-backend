#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "utf8proc::utf8proc" for configuration "Debug"
set_property(TARGET utf8proc::utf8proc APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(utf8proc::utf8proc PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/utf8proc.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/utf8proc.dll"
  )

list(APPEND _cmake_import_check_targets utf8proc::utf8proc )
list(APPEND _cmake_import_check_files_for_utf8proc::utf8proc "${_IMPORT_PREFIX}/debug/lib/utf8proc.lib" "${_IMPORT_PREFIX}/debug/bin/utf8proc.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
