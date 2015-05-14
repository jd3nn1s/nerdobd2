# - Find libgps
# Find the Gpsd includes and client library
# This module defines
#  GPSD_INCLUDE_DIR, where to find gps.h
#  GPSD_LIBRARIES, the libraries needed by a GPSD client.
#  GPSD_FOUND, If false, do not try to use GPSD.
# also defined, but not for general use are
#  GPSD_LIBRARY, where to find the GPSD library.

#=============================================================================
# Copyright 2010 Diego Berge <gpsd@navlost.eu>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, see <http://www.gnu.org/licenses/>
#=============================================================================

FIND_PATH(GPSD_INCLUDE_DIR gps.h)

SET(GPSD_NAMES ${GPSD_NAMES} gps)
FIND_LIBRARY(GPSD_LIBRARY NAMES ${GPSD_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set GPSD_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GPSD DEFAULT_MSG GPSD_LIBRARY GPSD_INCLUDE_DIR)

IF(GPSD_FOUND)
  SET(GPSD_LIBRARIES ${GPSD_LIBRARY})
ENDIF(GPSD_FOUND)

# Deprecated declarations.
SET (NATIVE_GPSD_INCLUDE_PATH ${GPSD_INCLUDE_DIR} )
IF(GPSD_LIBRARY)
  GET_FILENAME_COMPONENT (NATIVE_GPSD_LIB_PATH ${GPSD_LIBRARY} PATH)
ENDIF(GPSD_LIBRARY)

MARK_AS_ADVANCED(GPSD_LIBRARY GPSD_INCLUDE_DIR )
