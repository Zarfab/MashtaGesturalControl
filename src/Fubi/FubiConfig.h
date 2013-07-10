// ****************************************************************************************
//
// Fubi Config
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once

//<-----------------------------------------------------------------
// Essential config. Fubi might not work if there is something wrong here.

// Uncomment to use OpenNI version 1.x
//#define USE_OPENNI1

// Uncomment to use OpenNI version 2.x (default)
//#define USE_OPENNI2

// Uncomment to use the Kinect SDK
//#define USE_KINECT_SDK

// Define the number of users for the kinect sdk face tracking to be used
#define KINECT_SDK_MAX_NUM_FACES_TRACKED 2


// Note: without OpenCV active, you won't get debug information (e.g. the skeletons) rendered onto the depth image and finger count recognizers won't work!
// Uncomment to use most current supported OpenCV version, currently OpenCV 2.4.3 (default)
//#define USE_OPENCV
// Uncomment to use OpenCV version 2.2 (version with easy-to-use VS2010 installer)
//#define USE_OPENCV_22

//----------------------------------------------------------------->


//<-----------------------------------------------------------------
// Optional configs, for getting more debug infos, etc.

// Uncomment to print more information about the progress of the combination recognizers to the console
//#define COMBINATIONREC_DEBUG_LOGGING


// Log level options (Do not modify!)
// 0=all messages 1=errors, warnings, and infos 2= errors and warnings 3=errors and infos 4=errors only 5=silent
#define FUBI_LOG_VERBOSE 0
#define FUBI_LOG_ERR_WRN_INFO 1
#define FUBI_LOG_ERR_WRN 2
#define FUBI_LOG_ERR_INFO 3
#define FUBI_LOG_ERR 4
#define FUBI_LOG_SILENT 5

// Define log level (edit for debug or release configuration)
#ifdef _DEBUG
#define FUBI_LOG_LEVEL FUBI_LOG_VERBOSE
#else
#define FUBI_LOG_LEVEL FUBI_LOG_ERR_WRN_INFO
#endif

//----------------------------------------------------------------->
