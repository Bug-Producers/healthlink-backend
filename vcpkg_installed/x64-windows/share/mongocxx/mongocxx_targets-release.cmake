#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mongo::mongocxx_shared" for configuration "Release"
set_property(TARGET mongo::mongocxx_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mongo::mongocxx_shared PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/mongocxx-v_noabi-rhi-md.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "mongoc::shared"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/mongocxx-v_noabi-rhi-md.dll"
  )

list(APPEND _cmake_import_check_targets mongo::mongocxx_shared )
list(APPEND _cmake_import_check_files_for_mongo::mongocxx_shared "${_IMPORT_PREFIX}/lib/mongocxx-v_noabi-rhi-md.lib" "${_IMPORT_PREFIX}/bin/mongocxx-v_noabi-rhi-md.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
