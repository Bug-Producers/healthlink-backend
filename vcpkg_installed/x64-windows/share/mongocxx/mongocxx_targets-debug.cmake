#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mongo::mongocxx_shared" for configuration "Debug"
set_property(TARGET mongo::mongocxx_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mongo::mongocxx_shared PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/mongocxx-v_noabi-dhi-mdd.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "mongoc::shared"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/mongocxx-v_noabi-dhi-mdd.dll"
  )

list(APPEND _cmake_import_check_targets mongo::mongocxx_shared )
list(APPEND _cmake_import_check_files_for_mongo::mongocxx_shared "${_IMPORT_PREFIX}/debug/lib/mongocxx-v_noabi-dhi-mdd.lib" "${_IMPORT_PREFIX}/debug/bin/mongocxx-v_noabi-dhi-mdd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
