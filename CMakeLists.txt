cmake_minimum_required(VERSION 2.6)
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

project (nerdobd2)
use_cxx11()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(CMakeDependentOption)

set(SOURCES core.c kw1281.c common.h config.h core.h kw1281.h plugins.c plugins.h gps/gps.c gps/lowlevel.c gps/datalogger.c secondary_telemetry.c)

option (BUILD_HTTPD "Build internal HTTP server" ON)
option (USE_POSTGRESQL "Use PostgreSQL as database" OFF)
option (USE_SQLITE "Use SQLite as database" ON)
option (USE_GPSD "Use gpsd to get location" ON)

if (USE_POSTGRESQL)
  find_package(PostgreSQL REQUIRED)
  include_directories(${PostgreSQL_INCLUDE_DIRS})
  add_library(output_postgres SHARED output_postgres.c output_postgres.h common.h plugins.h db_common.c db_common.h postgres2json.c json.c json.h)
  target_link_libraries(output_postgres ${PostgreSQL_LIBRARIES})
endif (USE_POSTGRESQL)

if (USE_SQLITE)
  find_package(SQLite3 REQUIRED)
  include_directories(${SQLITE3_INCLUDE_DIRS})
  add_library(output_sqlite SHARED output_sqlite.c output_sqlite.h common.h plugins.h db_common.c db_common.h sqlite2json.c json.c json.h)
  target_link_libraries(output_sqlite ${SQLITE3_LIBRARIES})
endif (USE_SQLITE)

find_package(JSON-C REQUIRED)
include_directories(${JSON-C_INCLUDE_DIRS})
list(APPEND EXTRA_LIBS ${JSON-C_LIBRARIES})

if (USE_GPSD)
  find_package(GPSD REQUIRED)
  include_directories(${GPSD_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS ${GPSD_LIBRARIES})
  list(APPEND SOURCES gps/gpsd.c gps/gpsd.h)
endif (USE_GPSD)

if (BUILD_HTTPD)
  include_directories(${CMAKE_SOURCE_DIR}/externals/mongoose)
  add_library(mongoose externals/mongoose/mongoose.c externals/mongoose/mongoose.h)
  list(APPEND EXTRA_LIBS mongoose)
  list(APPEND SOURCES httpd.c httpd.h)
endif (BUILD_HTTPD)

include_directories(${CMAKE_SOURCE_DIR}/externals/inih)
add_library(ini externals/inih/ini.c externals/inih/ini.h)
list(APPEND EXTRA_LIBS ini)

add_subdirectory(${CMAKE_SOURCE_DIR}/externals/vag-rosetta)
include_directories(${CMAKE_SOURCE_DIR}/externals/vag-rosetta)

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

add_executable(nerdobd2 ${SOURCES})
target_link_libraries (nerdobd2 ${EXTRA_LIBS} pthread dl m rt vag-rosetta)

add_library(output_csv SHARED output_csv.c common.h plugins.h)

add_library(easywsclient externals/easywsclient/easywsclient.hpp externals/easywsclient/easywsclient.cpp)
set_target_properties(easywsclient PROPERTIES COMPILE_FLAGS "-fPIC")
include_directories(${CMAKE_SOURCE_DIR}/externals/easywsclient)

add_library(output_websocket SHARED output_websocket.cpp common.h plugins.h)
target_link_libraries(output_websocket ${CMAKE_SOURCE_DIR}/libeasywsclient.a)

add_library(output_udp SHARED output_udp.cpp common.h plugins.h)
add_library(output_canbus SHARED output_canbus.c common.h plugins.h)

set(CMAKE_BUILD_TYPE Debug)
