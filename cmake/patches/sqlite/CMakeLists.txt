cmake_minimum_required(VERSION 3.25)

project(sqlite)

set(SQLITE_SRCS
    "sqlite3.c"
)

set(SQLITE_INCLUDES
    "sqlite3.h"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(sqlite ${SQLITE_SRCS})

# take the hit, always ensure thread safety
target_compile_definitions(sqlite PRIVATE SQLITE_THREADSAFE=1)
# C++11
target_compile_features(sqlite PRIVATE cxx_rvalue_references)

install(TARGETS sqlite
  LIBRARY DESTINATION lib COMPONENT RuntimeLibraries
  ARCHIVE DESTINATION lib COMPONENT Development)

foreach(SQLITE_INCLUDE ${SQLITE_INCLUDES})
  install(FILES ${SQLITE_INCLUDE} DESTINATION include COMPONENT Development)
endforeach()

