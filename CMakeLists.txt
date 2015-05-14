cmake_minimum_required(VERSION 2.6)
project (nerdobd2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(CMakeDependentOption)

set(SOURCES core.c httpd.c json.c tcp.c kw1281.c)

option (DB_POSTGRES
        "Build PostgreSQL support with user and database name" "user=nerdobd2 dbname=nerdobd2")
option (DB_SQLITE
	"Build SQLite support using database name" database.sqlite3)

option (USE_POSTGRESQL "Use PostgreSQL as database" OFF)
cmake_dependent_option(USE_SQLITE "Use SQLite as database" OFF "NOT USE_POSTGRESQL" OFF)

if (NOT USE_POSTGRESQL AND NOT USE_SQLITE)
  message(FATAL_ERROR "You must set USE_POSTGRESQL or USE_SQLITE")
endif (NOT USE_POSTGRESQL AND NOT USE_SQLITE)

if (USE_POSTGRESQL)
  find_package(PostgreSQL REQUIRED)
  include_directories(${PostgreSQL_INCLUDE_DIRS})
  list(APPEND SOURCES postgres.c postgres2json.c postgres.h)
  list(APPEND EXTRA_LIBS ${PostgreSQL_LIBRARIES})
endif (USE_POSTGRESQL)

if (USE_SQLITE)
  find_package(SQLite3 REQUIRED)
  include_directories(${SQLITE3_INCLUDE_DIRS})
  list(APPEND SOURCES sqlite.c sqlite2json.c postgres.h)
  list(APPEND EXTRA_LIBS ${SQLITE3_LIBRARIES})
endif (USE_SQLITE)

find_package(JSON-C REQUIRED)
include_directories(${JSON-C_INCLUDE_DIRS})
list(APPEND EXTRA_LIBS ${JSON-C_LIBRARIES})

find_package(GPSD)

if (GPSD_FOUND)
  include_directories(${GPSD_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS ${GPSD_LIBRARIES})
  list(APPEND SOURCES gps.c)
endif (GPSD_FOUND)

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

add_executable(nerdobd2 ${SOURCES})
target_link_libraries (nerdobd2 ${EXTRA_LIBS})
