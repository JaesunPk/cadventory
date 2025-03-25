include (ExternalProject)

set(sqlite_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/sqlite/install/include)
set(sqlite_URL https://sqlite.org/2024/sqlite-amalgamation-3450200.zip)
set(sqlite_HASH SHA256=65230414820d43a6d1445d1d98cfe57e8eb9f7ac0d6a96ad6932e0647cce51db)
set(sqlite_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sqlite/src/sqlite)
set(sqlite_INSTALL ${CMAKE_CURRENT_BINARY_DIR}/sqlite/install)

if(WIN32)
  set(sqlite_STATIC_LIBRARIES ${sqlite_INSTALL}/lib/sqlite.lib)
else()
  set(sqlite_STATIC_LIBRARIES ${sqlite_INSTALL}/lib/libsqlite.a)
endif()

set(sqlite_HEADERS
  "${sqlite_BUILD}/sqlite3.h"
)

ExternalProject_Add(sqlite
  PREFIX sqlite
  URL ${sqlite_URL}
  URL_HASH ${sqlite_HASH}
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/sqlite/CMakeLists.txt ${sqlite_BUILD}
  INSTALL_DIR ${sqlite_INSTALL}
  DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
  CMAKE_ARGS -DCMAKE_OSX_ARCHITECTURES=arm64
  CMAKE_CACHE_ARGS
  -DCMAKE_BUILD_TYPE:STRING=Release
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
  -DCMAKE_INSTALL_PREFIX:STRING=${sqlite_INSTALL}
)

# put sqlite includes in the directory where they are expected
add_custom_target(sqlite_create_destination_dir
  COMMAND ${CMAKE_COMMAND} -E make_directory ${sqlite_INCLUDE_DIR}
  DEPENDS sqlite)

add_custom_target(sqlite_copy_headers_to_destination
  DEPENDS sqlite_create_destination_dir)

foreach(header_file ${sqlite_HEADERS})
  add_custom_command(TARGET sqlite_copy_headers_to_destination PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header_file} ${sqlite_INCLUDE_DIR})
endforeach()
