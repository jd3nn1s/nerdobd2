cmake_minimum_required(VERSION 2.6)
project (nerdobd2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(CMakeDependentOption)

set(SOURCES core.c json.c tcp.c kw1281.c common.h config.h core.h json.h kw1281.h tcp.h plugins.c plugins.h)

option (BUILD_HTTPD "Build internal HTTP server" ON)
option (USE_POSTGRESQL "Use PostgreSQL as database" OFF)
option (USE_GPSD "Use gpsd to get location" ON)
cmake_dependent_option(USE_SQLITE "Use SQLite as database" OFF "NOT USE_POSTGRESQL" OFF)

if (NOT USE_POSTGRESQL AND NOT USE_SQLITE)
  message(FATAL_ERROR "You must set USE_POSTGRESQL or USE_SQLITE")
endif (NOT USE_POSTGRESQL AND NOT USE_SQLITE)

if (USE_POSTGRESQL)
  find_package(PostgreSQL REQUIRED)
  include_directories(${PostgreSQL_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS ${PostgreSQL_LIBRARIES})
  add_library(output_postgres SHARED output_postgres.c output_postgres.h common.h plugins.h db_common.h)
endif (USE_POSTGRESQL)

if (USE_SQLITE)
  find_package(SQLite3 REQUIRED)
  include_directories(${SQLITE3_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS ${SQLITE3_LIBRARIES})
  add_library(output_sqlite SHARED output_sqlite.c output_sqlite.h common.h plugins.h db_common.h)
endif (USE_SQLITE)

find_package(JSON-C REQUIRED)
include_directories(${JSON-C_INCLUDE_DIRS})
list(APPEND EXTRA_LIBS ${JSON-C_LIBRARIES})

if (USE_GPSD)
  find_package(GPSD REQUIRED)
  include_directories(${GPSD_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS ${GPSD_LIBRARIES})
  list(APPEND SOURCES gps.c gps.h)
endif (USE_GPSD)

if (BUILD_HTTPD)
  list(APPEND SOURCES httpd.c httpd.h)
  if (USE_SQLITE)
    list(APPEND SOURCES sqlite.c sqlite.h sqlite2json.c)
  endif (USE_SQLITE)
  if (USE_POSTGRESQL)
    list(APPEND SOURCES postgres.c postgres.h postgres2json.c)
  endif (USE_POSTGRESQL)
endif (BUILD_HTTPD)

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

add_executable(nerdobd2 ${SOURCES})
target_link_libraries (nerdobd2 ${EXTRA_LIBS} dl)

add_library(output_test SHARED output_test.c common.h plugins.h)
