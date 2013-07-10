macro(ADD_QT_EXECUTABLE TARGET_NAME)

	SET(WITH_QT4 ON)

	FILE(GLOB ${TARGET_NAME}_SRCS ${CMAKE_CURRENT_BINARY_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
	FILE(GLOB ${TARGET_NAME}_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_BINARY_DIR}/*.h)
	FILE(GLOB ${TARGET_NAME}_UIS ${CMAKE_CURRENT_SOURCE_DIR}/*.ui ${CMAKE_CURRENT_BINARY_DIR}/*.ui)

	# Generates ui_*.h files
	QT4_WRAP_UI(${TARGET_NAME}_UIS_H ${${TARGET_NAME}_UIS})

	# Generates moc_*.cxx files
	QT4_WRAP_CPP(${TARGET_NAME}_MOC_SRCS ${${TARGET_NAME}_HDRS} OPTIONS "-nw") # for all headers that potentially declare Q_OBJECT, otherwise warnings are suppressed

	# Don't forget to include output directory, otherwise
	# the UI file won't be wrapped!
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

        IF(NOT ICON_NAME)
                SET(ICON_NAME "FUBIOSC")
        ENDIF()

	IF ( WIN32 OR MINGW)
		EXECUTE_PROCESS(COMMAND echo "IDI_ICON1	ICON	DISCARDABLE \"${CMAKE_SOURCE_DIR}/${ICON_NAME}.ico\"" OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.rc)
		EXECUTE_PROCESS(COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.rc 
                             -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o)
		IF(USE_DEBUG)
			SET(OS_SPECIFIC ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o)
		ELSE()
			#SET(OS_SPECIFIC "WIN32" ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o)
			SET(OS_SPECIFIC ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o)
		ENDIF()
	ELSE ()
		IF ( APPLE )
			SET(APP_TYPE MACOSX_BUNDLE)
			SET(MACOSX_BUNDLE_BUNDLE_NAME ${TARGET_NAME})
			# set how it shows up in the Info.plist file
                        SET(MACOSX_BUNDLE_ICON_FILE ${ICON_NAME}.icns)
                        # set the bundle identifier (REQUIRED, or some strange GUI bugs may appear)
                        SET(MACOSX_BUNDLE_GUI_IDENTIFIER "org.numediart.${TARGET_NAME}")
                        SET(MACOSX_BUNDLE_INFO_STRING "${TARGET_NAME}")
                        SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${TARGET_NAME}")
                        SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${TARGET_NAME}")
                        #SET(MACOSX_BUNDLE_BUNDLE_VERSION "0.1")
                        SET(MACOSX_BUNDLE_COPYRIGHT "Copyright UMONS 2012-present")
  			# set where in the bundle to put the icns file
  			SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/${ICON_NAME}.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  			# include the icns file in the target
  			SET(${TARGET_NAME}_SRCS ${${TARGET_NAME}_SRCS} ${CMAKE_SOURCE_DIR}/${ICON_NAME}.icns)
			SET(OS_SPECIFIC ${APP_TYPE} ${MACOSX_BUNDLE_INFO_PLIST})
		ENDIF()
	ENDIF()
	ADD_EXECUTABLE(${TARGET_NAME} ${OS_SPECIFIC} ${${TARGET_NAME}_SRCS} ${${TARGET_NAME}_HDRS} ${${TARGET_NAME}_UIS_H} ${${TARGET_NAME}_MOC_SRCS})

        IF(USE_BREAKPAD)
            ADD_DEPENDENCIES(${TARGET_NAME} breakpad-qt)
            TARGET_LINK_LIBRARIES(${TARGET_NAME} breakpad-qt)
        ENDIF()
        TARGET_LINK_LIBRARIES(${TARGET_NAME} ${QT_LIBRARIES})
        IF(NOT MC_PLUGINS)
            TARGET_LINK_LIBRARIES(${TARGET_NAME} ${OPENGL_LIBRARIES})
        ENDIF()

	# Copy Qt imageformat plugin to (Apple) Debug apps
	IF(APPLE AND USE_DEBUG)
		IF(XCODE)
			SET(EXTRA_PATH "/Debug")
		ENDIF()
		ADD_CUSTOM_COMMAND( TARGET ${TARGET_NAME} POST_BUILD
			COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}${EXTRA_PATH}/${TARGET_NAME}.app/Contents/plugins
      			COMMAND cp -R ${QT_PLUGINS_DIR}/imageformats ${CMAKE_CURRENT_BINARY_DIR}${EXTRA_PATH}/${TARGET_NAME}.app/Contents/plugins
		)
	ENDIF()
endmacro(ADD_QT_EXECUTABLE TARGET_NAME)
