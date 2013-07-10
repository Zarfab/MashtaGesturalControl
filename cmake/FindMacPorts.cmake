# - Find MacPorts
# Find the native MacPorts executable, determine variants
# This module defines
#  MACPORTS_FOUND, if false, do not try to use MacPorts.
#  MACPORTS_PORT, the port command with full path.
#  MACPORTS_PREFIX, the installation prefix of MacPorts.
#  MACPORTS_CONF, the location of macports.conf (default: /opt/local/etc/macports/macports.conf)
#  MACPORTS_VARIANTS_CONF, the location of variants.conf (default: /opt/local/etc/macports/variants.conf)
#  MACPORTS_BUILD_ARCH, the default architecture(s) for ports, can be used for CMAKE_OSX_ARCHITECTURES
#  MACPORTS_UNIVERSAL_ARCHS, the default universal architecture(s) for ports 
#  MACPORTS_UNIVERSAL_FLAG_SET, TRUE if +universal is defined in variants.conf

#=============================================================================
# Copyright 2012 Christian Frisson UMONS
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

MESSAGE(STATUS "Checking for MacPorts installation")
IF(APPLE)
	SET(MACPORTS_VARIANTS_CONF)
	SET(MACPORTS_CONF)
	FIND_PROGRAM(MACPORTS_PORT NAMES port)
	FIND_FILE(MACPORTS_CONF NAME macports.conf PATH_SUFFIXES etc/macports)
	IF(MACPORTS_CONF AND EXISTS "${MACPORTS_CONF}")
		file(READ "${MACPORTS_CONF}" MACPORTS_CONF_CONTENTS)
		STRING(REGEX MATCH ".*variants_conf[ \t]+[/0-9a-z.]+.*" MACPORTS_VARIANTS_CONF_LISTED "${MACPORTS_CONF_CONTENTS}")
		IF(MACPORTS_VARIANTS_CONF_LISTED)
			STRING(REGEX REPLACE ".*variants_conf[ \t]+([/0-9a-z.]+).*" "\\1" MACPORTS_VARIANTS_CONF ${MACPORTS_CONF_CONTENTS})
			MESSAGE(STATUS "MacPorts variants.conf: ${MACPORTS_VARIANTS_CONF}")
		ENDIF()

		STRING(REGEX MATCH ".*prefix[ \t]+[/0-9a-z.]+.*" MACPORTS_PREFIX_LISTED "${MACPORTS_CONF_CONTENTS}")
		IF(MACPORTS_PREFIX_LISTED)
			STRING(REGEX REPLACE ".*prefix[ \t]+([/0-9a-z.]+).*" "\\1" MACPORTS_PREFIX ${MACPORTS_CONF_CONTENTS})
		MESSAGE(STATUS "MacPorts prefix: ${MACPORTS_PREFIX}")
		ENDIF()

		STRING(REGEX MATCH ".*[^#][ \t]*build_arch[ \t]+[0-9a-z_ ]+.*" MACPORTS_BUILD_ARCH_LISTED "${MACPORTS_CONF_CONTENTS}")
		IF(MACPORTS_BUILD_ARCH_LISTED)
			STRING(REGEX REPLACE ".*[^#][ \t]*build_arch[ \t]+([0-9a-z_ ]+).*" "\\1" MACPORTS_BUILD_ARCH ${MACPORTS_CONF_CONTENTS})
		ELSE()
			# CPU architecture to compile for. Defaults to i386 or ppc on Mac OS X 10.5
			# and earlier, depending on the CPU type detected at runtime. On Mac OS X 10.6
			# the default is x86_64 if the CPU supports it, i386 otherwise.
			IF(${CMAKE_SYSTEM_VERSION} LESS "10.") # Mac OS X 10.5 and earlier
				SET(MACPORTS_BUILD_ARCH "i386")
			ELSE()
				SET(MACPORTS_BUILD_ARCH "x86_64")
			ENDIF()
		ENDIF()
		MESSAGE(STATUS "MacPorts build arch: ${MACPORTS_BUILD_ARCH}")

		STRING(REGEX MATCH ".*[^#][ \t]*universal_archs[ \t]+[0-9a-z_ ]+.*" MACPORTS_UNIVERSAL_ARCH_LISTED "${MACPORTS_CONF_CONTENTS}")
		IF(MACPORTS_UNIVERSAL_ARCH_LISTED)
			STRING(REGEX REPLACE ".*[^#][ \t]*universal_archs[ \t]+([0-9a-z_ ]+).*" "\\1" MACPORTS_UNIVERSAL_ARCHS ${MACPORTS_CONF_CONTENTS})
		ELSE()
			IF(${CMAKE_SYSTEM_VERSION} LESS "10.") # Mac OS X 10.5 and earlier
				SET(MACPORTS_UNIVERSAL_ARCHS "i386")
			ELSE()
				SET(MACPORTS_UNIVERSAL_ARCHS "x86_64 i386")
			ENDIF()
		ENDIF()
		MESSAGE(STATUS "MacPorts universal arch: ${MACPORTS_UNIVERSAL_ARCHS}")

 		IF(MACPORTS_VARIANTS_CONF AND EXISTS "${MACPORTS_VARIANTS_CONF}")
			file(READ "${MACPORTS_VARIANTS_CONF}" MACPORTS_VARIANTS_CONF_CONTENTS)
			STRING(REGEX MATCH ".*[^#][ \t+]universal.*" MACPORTS_UNIVERSAL_FLAG_SET "${MACPORTS_VARIANTS_CONF_CONTENTS}")
		ENDIF()
		IF(MACPORTS_UNIVERSAL_FLAG_SET)
			SET(MACPORTS_UNIVERSAL_FLAG_SET CACHE BOOL ON)
			MESSAGE(STATUS "MacPorts universal flag set")
		ELSE()
			SET(MACPORTS_UNIVERSAL_FLAG_SET CACHE BOOL OFF)
			MESSAGE(STATUS "MacPorts universal flag NOT set")
		ENDIF()
		
	ENDIF()

	# handle the QUIETLY and REQUIRED arguments and set MATIO_FOUND to TRUE if 
	# all listed variables are TRUE
	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(MACPORTS DEFAULT_MSG MACPORTS_PORT MACPORTS_CONF MACPORTS_VARIANTS_CONF)

	MARK_AS_ADVANCED(MACPORTS_PORT MACPORTS_PREFIX MACPORTS_CONF MACPORTS_VARIANTS_CONF MACPORTS_BUILD_ARCH MACPORTS_UNIVERSAL_ARCHS MACPORTS_UNIVERSAL_FLAG_SET)

ELSE(APPLE)
	MESSAGE(STATUS "MacPorts only available for Apple OSX")
ENDIF(APPLE)