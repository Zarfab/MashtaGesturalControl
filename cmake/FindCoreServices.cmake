# Locate Apple CoreServices
# This module defines
# CORESERVICES_LIBRARY
# CORESERVICES_FOUND, if false, do not try to link to gdal 
# CORESERVICES_INCLUDE_DIR, where to find the headers
#
# $CORESERVICES_DIR is an environment variable that would
# correspond to the ./configure --prefix=$CORESERVICES_DIR
#
# Created by Christian Frisson.

IF(APPLE)
  FIND_PATH(CORESERVICES_INCLUDE_DIR CoreServices/CoreServices.h)
  FIND_LIBRARY(CORESERVICES_LIBRARY CoreServices)
ENDIF()


SET(CORESERVICES_FOUND "NO")
IF(CORESERVICES_LIBRARY AND CORESERVICES_INCLUDE_DIR)
  SET(CORESERVICES_FOUND "YES")
ENDIF()

