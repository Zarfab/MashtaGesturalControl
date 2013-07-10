# - Find OpenNI
# Find the native OPENNI includes and library
# This module defines
#  OPENNI_INCLUDE_DIR, where to find XnOPENNI.h, etc.
#  OPENNI_LIBRARIES, the libraries needed to use OPENNI.
#  OPENNI_FOUND, If false, do not try to use OPENNI.
# also defined, but not for general use are
#  OPENNI_LIBRARY, where to find the OPENNI library.
#  OPENNI_VERSION, the version of the OPENNI library.

#=============================================================================
# Copyright 2012 UMONS, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

FIND_PATH(OPENNI_INCLUDE_DIR NAMES XnOpenNI.h OpenNi.h PATH_SUFFIXES "ni" "OpenNI")

FIND_LIBRARY(OPENNI_LIBRARY NAMES OpenNI OpenNI2)

# handle the QUIETLY and REQUIRED arguments and set OPENNI_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENNI DEFAULT_MSG OPENNI_LIBRARY OPENNI_INCLUDE_DIR)

IF(OPENNI_FOUND)
  SET(OPENNI_LIBRARIES ${OPENNI_LIBRARY})
  GET_FILENAME_COMPONENT(OPENNI_LINK_DIRECTORIES ${OPENNI_LIBRARY} PATH)
ENDIF(OPENNI_FOUND)

# Deprecated declarations.
SET (NATIVE_OPENNI_INCLUDE_PATH ${OPENNI_INCLUDE_DIR} )
IF(OPENNI_LIBRARY)
  GET_FILENAME_COMPONENT (NATIVE_OPENNI_LIB_PATH ${OPENNI_LIBRARY} PATH)
ENDIF(OPENNI_LIBRARY)

MARK_AS_ADVANCED(OPENNI_LIBRARY OPENNI_INCLUDE_DIR OPENNI_LINK_DIRECTORIES)

# Try to ascertain the version...
FIND_PATH(OPENNI_VERSION_PATH NAME XnVersion.h PATH_SUFFIXES "ni" "OpenNI")
FIND_PATH(OPENNI2_VERSION_PATH NAME OniVersion.h PATH_SUFFIXES "ni" "OpenNI")
MESSAGE("OPENNI2_VERSION_PATH ${OPENNI2_VERSION_PATH}")
if(OPENNI_VERSION_PATH OR OPENNI2_VERSION_PATH)
	IF(OPENNI_VERSION_PATH)
		SET(OPENNI_VERSION_HEADER "${OPENNI_VERSION_PATH}/XnVersion.h")
		SET(VERSION_MAJOR "XN_MAJOR_VERSION")
		SET(VERSION_MINOR "XN_MINOR_VERSION")
		SET(VERSION_MAINTENANCE "XN_MAINTENANCE_VERSION")
		SET(VERSION_BUILD "XN_BUILD_VERSION")
	ELSEIF(OPENNI2_VERSION_PATH)
		SET(OPENNI_VERSION_HEADER "${OPENNI2_VERSION_PATH}/OniVersion.h")
		SET(VERSION_MAJOR "ONI_VERSION_MAJOR")
		SET(VERSION_MINOR "ONI_VERSION_MINOR")
		SET(VERSION_MAINTENANCE "ONI_VERSION_MAINTENANCE")
		SET(VERSION_BUILD "ONI_VERSION_BUILD")
	ENDIF()
	if(EXISTS "${OPENNI_VERSION_HEADER}")
        	file(READ "${OPENNI_VERSION_HEADER}" _ni_Version_contents)
    		string(REGEX MATCH ".*#define ${VERSION_MAJOR}[ \t]+[0-9]+.*"
			_ni_new_version_defines "${_ni_Version_contents}")
		if(_ni_new_version_defines)
			string(REGEX REPLACE ".*#define ${VERSION_MAJOR}[ \t]+([0-9]+).*"
				"\\1" OPENNI_VERSION_MAJOR ${_ni_Version_contents})
			string(REGEX REPLACE ".*#define ${VERSION_MINOR}[ \t]+([0-9]+).*"
				"\\1" OPENNI_VERSION_MINOR ${_ni_Version_contents})
			string(REGEX REPLACE ".*#define ${VERSION_MAINTENANCE}[ \t]+([0-9]+).*"
				"\\1" OPENNI_VERSION_MAINTENANCE ${_ni_Version_contents})
			string(REGEX REPLACE ".*#define ${VERSION_BUILD}[ \t]+([0-9]+).*"
				"\\1" OPENNI_VERSION_BUILD ${_ni_Version_contents})
		else()
			message(FATAL_ERROR "[ FindOpenNI.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
				"Failed to parse version number, please report this as a bug")
		endif()
	endif()

    set(OPENNI_VERSION "${OPENNI_VERSION_MAJOR}.${OPENNI_VERSION_MINOR}.${OPENNI_VERSION_MAINTENANCE}.${OPENNI_VERSION_BUILD}"
                                CACHE INTERNAL "The version of OpenNI which was detected")
    MARK_AS_ADVANCED(OPENNI_VERSION_MAJOR OPENNI_VERSION_MINOR OPENNI_VERSION_MAINTENANCE OPENNI_VERSION_BUILD)
endif()

if(OpenNI_FIND_VERSION AND OPENNI_VERSION)
    if(OpenNI_FIND_VERSION_EXACT)
        if(NOT OPENNI_VERSION VERSION_EQUAL ${OpenNI_FIND_VERSION})
            MESSAGE(FATAL_ERROR "OpenNI version found is ${OPENNI_VERSION}, while version needed is exactly ${OpenNI_FIND_VERSION}.")
        endif()
    else()
        # version is too low
        if(NOT OPENNI_VERSION VERSION_EQUAL ${OpenNI_FIND_VERSION} AND 
                NOT OPENNI_VERSION VERSION_GREATER ${OpenNI_FIND_VERSION})
            MESSAGE(FATAL_ERROR "OpenNI version found is ${OPENNI_VERSION}, while version needed is ${OpenNI_FIND_VERSION}.")
        endif()
    endif()
endif()


