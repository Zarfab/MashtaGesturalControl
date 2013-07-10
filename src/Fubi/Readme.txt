=================================
             FUBI 
(Full Body Interaction Framework)
=================================

Version 0.7.0


Copyright (C) 2010-2013 Felix Kistler

http://www.hcm-lab.de/fubi.html


The framework is licensed under the terms of the Eclipse Public License (EPL).

FUBI makes use of the following third-party open source software included in code form:
 - RapidXml (http://rapidxml.sourceforge.net)
 
FUBI can be used with the following third-party open source software not included in the FUBI download:
 - OpenCV (http://opencv.willowgarage.com)
 - OpenNI (httl://www.openni.org)

FUBI can be used with the following third-party closed source software not included in the FUBI download:
 - Microsoft Kinect SDK (http://www.microsoft.com/en-us/kinectforwindows/)
 - NiTE (http://www.openni.org/files/nite/)
 
A documentation with pictures and more detailed tutorials can be found here:
http://www.informatik.uni-augsburg.de/lehrstuehle/hcm/projects/tools/fubi/doc/


Installation of third-party components
======================================

You need to install the following third-party components:
1. OpenNI:
 http://www.openni.org/openni-sdk/
 --> OpenNI Binaries --> OpenNI 2.xx (Win32) --> extract and run OpenNI-Windows-x86-2.xx.msi
2. NITE:
 http://www.openni.org/files/nite/
 --> Download (You need to register for this) --> extract and run NiTE-Windows-x86-2.xx.msi
3. Sensor driver: --IMPORTANT-- Install this AFTER the other two installations!
 a) ASUS Xtion (Pro live): Should be automatically installed with OpenNI.
 c) Kinect for Xbox/Windows: See 4. Kinect SDK 1.x

Alternatively or in addition you can install:
4. Kinect SDK 1.x:
 http://www.microsoft.com/en-us/kinectforwindows/develop/developer-downloads.aspx
 --> Download and install the latest Kinect for Windows SDK and Kinect for Windows Developer Toolkit

5. We recommend to additionally install OpenCV, currently, the latest supported version is 2.4.3:
For Windows: http://sourceforge.net/projects/opencvlibrary/files/opencv-win/2.4.3/OpenCV-2.4.3.exe/download
If you do not want to use OpenCV, comment out the line "#define USE_OPENCV" at top of the FubiConfig.h
If you already have installed OpenCV 2.2 with an earlier release, you can also use that: You have to comment out the line "#define USE_OPENCV" and uncomment "#define USE_OPENCV_22" at top of the FubiConfig.h


Running a FUBI sample
=====================

1. Open the FUBI.sln
2. Set "RecognizerTest" (in the sample folder) as startup project, compile, and run it

The application will start, showing the depth image of the Kinect with additional debug info for the user tracking.
The application as default checks for all implemented posture recognizers and prints the recognition on the console.
If you press "space", it will instead print the current state of all combination recognizers to the console.
If you press "space" again, it will print the current state of all user defined recognizers.
And if you press "space" once again, it will print the current state of the pointing with right hand recognizer.


Available keys:
---------------
ESC 	: Shutdown the application
Space 	: Switch between posture and combination output (for the two closest users)
p		: Save pictures of the Kinect every 30 frames (1-2 sec.): 1 rgb image, 1 depth image, 1 tracking image
f		: Activate the finger count detection for the closest standing user every 15 frames (0,5-1 sec.)
r/i		: Switch to RGB or IR image (needs proper configuration in the Fubi::init() call at the top of the main function)
t		: Switch between different rendering options.
s		: Switch between different sensors (e.g. Kinect SDK and OpenNI).
TAB		: Reload recognizers from the "SampleRecognizers.xml"


Note the sample is tested with OpenNI 2.0.0, NITE 2.0.0, OpenCV 2.4.3, as well as the Kinect SDK

Defining a recognizer in XML
============================
We recommend to use an XML editor that is able to use *.dtd files for XML schemes (e.g. Visual Studio on Windows).
If you open one of the existing XML sample files (e.g. "SampleRecognizers.xml" in the bin folder), your editor should automatically load the FubiRecognizers.dtd to support you while editing it.
More information can be found in the online documentation (http://www.informatik.uni-augsburg.de/lehrstuehle/hcm/projects/tools/fubi/doc/).



Implementation of a posture recognizer in code
==============================================

You can take the example recognizer "LeftHandUpRecognizer" in the folder "Fubi\GestureRecognizer" as a template.
It implements the interface of "Fubi\GestureRecognizer\IGestureRecognizer.h"
In "Fubi\Fubi.h" you have to add a value for your recognizer in the enum "Postures" and a description string in "getPostureName(Postures postureID)".
"Fubi\FubiRecognizerFactory.cpp" serves as a factory, and you have to link your enum-value with your recognizer's constructor.



Implementation of a combination recognizer in code
==================================================

Your combination recognizer should be implemented in "Fubi\FubiRecognizerFactory.cpp" following the example "WAVE_RIGHT_HAND_OVER_SHOULDER".
A combination consists of several states. Each state consists of a set of recognizers that need to be succesful for going into this state. Each state has a minimum duration that all gestures have to be performed before proceeding to the next sate. Each state has a maximum duration, after which the recognition is aborted, if there has not been a transition to the next state, yet.
At last, each state has a time for transition that is the maximum duration between holding the postures of the current and the next state (where both states are not fulfilled).
In "Fubi\Fubi.h" you have to add a value for your recognizer in the enum "Combinations" and a description string in "getCombinationName(Combinations postureID)".



Implementation of a recognizers in code outside of FUBI
=======================================================

You can add linear movements and user defined posture recognizers with the functions addLinearMovementRecognizer(..) and addJointRelationRecognizer(..). They both return an index of the recognizer that has to be used to call it later.


-------------------------------------------------------------------------------------------------------
CHANGELOG
-------------------------------------------------------------------------------------------------------


Changes in 0.2.1
================

- C#-Wrapper with sample GUI
- XML-Configuration for recognizers (You can look up the format in the FubiRecognizers.dtd)
- Counting shown fingers by the users (beta)

Changes in 0.2.2
================

- C# Sample GUI now includes a mouse simulator:
 * You can start the mouse tracking by clicking on the button "start mouse emulation" or by the activation gesture "waving" if the corresponding check box is activated
 * You can switch between right and left hand control. Right hand control means you control the mouse cursor by moving the right hand in the air.
 * Clicking is done via gestures. These gestures are configured in the "MouseControlRecognizers.xml". You can change them, but you have to keep the names "mouseClick" and "mouseClick1"
- FUBI API: getClosestUsers returns the users standing closest to the sensor, but still in view and tracked.

Changes in 0.2.3
=================

- fixed bug with using elbows in xml recognizers
- a relative joint for linear movements is now optional, so movements can be measured absolute (usefull for whole body movements)
- added JointOrientationRecognizers for recognition of specific joint orientations
- new recognizers: left/right hand close to arm

Changes in 0.2.4
==================
-added new xml attribute for joint relations/linear movements/joint orientations: visibility. Default is visible. If set to hidden the recognizer won't be accessible from the outside and can only be used in combinations.
-added support for setting the openni skeleton profile.
-added local joint orientations
-updated C#-Wrapper and examples

Changes in 0.3.0
================
-added option for drawing local joint orientations in the depth image
-added new method for finger detection using morphological opening
-splitted update and getImage() function
-added new saveImage function
-much more options for image rendering
-added local positions and render options for local/global positions/orientations
-added function for directly injecting joint positions/orientations, e.g. enables direct integration with the MS Kinect SDK

Changes in 0.4.0
================
-Several API changes for the gesture recognizer calls: different names and now returning a Fubi::RecognitionResult instead of a simple bool
-new options for combination recognizers: "ignoreOnTrackingError" for specific recognizers within one state of the recognition for making them more "optional", and "maxInterruptionTime" for defining the maximum time the recognition within one state may be interrupted
-new recognizer type: finger count recognizer
-init now returns whether it was succesful, calibration files are no longer supported
-Several OpenNI only functions are now named accordingly
-Added new sample for integrating the MSKinect SDK with FUBI
-FUBI WPF GUI sample: fixed threading for Fubi update calls

Changes in 0.5.0
================
-Replaced OpenNI data types with Fubi specific ones to get more independet from OpenNI
-Added realWorldToProjective() function even without OpenNI
-Removed center of mass - not used in the past anyway. If you want to get a user's position even before tracking, look at the torso joint
-Fixed orientation recognizers that have the +180/-180 between a min and max value (e.g. minDegrees.x = -170, maxDegrees.x = -170 = 190).
-Added more getClosestUsers functions.
-Separated OpenNi completely from the rest using the FubiOpenNISensor class and a FubiISensor interface.
-getImage() can now be used with a user and joint of interest as saveImage()
-Added init witout xml file, but with Fubi::SensorOptions
-Renamed PostureCombinationRecognizers to the more appropriate CombinationRecognizer and all corresponding functions and the xml scheme
-Added struct around the predefined recognizers so that they are not directly in the Fubi root namespace anymore
-minConfidence now configurable per recognizer or globally via xml
-Added Unity3D integration and sample

Changes in 0.5.1
================
No API changes this time, about several internal ones:
-Added FubiConfig.h for configuring which OpenNI and OpenCV version to use and printing additional info for the combination recognizers
-Fubi is now tested with OpenCV 2.4.3
-Added swap r and b option and finger shapes for rendering
-Added minconfidence for all recognizers and maxAngleDiff for linear movements to the FUBI XML definition for setting
-Fubi is now using the new OpenNI version 2.0.0 by default!

Changes in 0.6.0
================
- Added full support for the Kinect SDK including face tracking (can still be deactivated via preprocessor defines in FubiConfig.h)
-> Added options to switch between sensors during run time
-> Added rendering options for face tracking
-> Removed MSKinectSDKTest sample from Visual Studio solution (Still available in the samples folder under "TrackingDataInjectionTest")
- Fixed finger rendering option
- Fixed a bug in the samples causing too many gesture notifications
- Fubi coordinate sytem is now fully right-handed (also for orientations), only the y orientation is rotated by 180° to have the 0 orientations when looking directly to the sensor (-> Changes needed for orientation recognizers!)
- Several new options for the recognizer xml:
 * combination recognizers can now be delayed until the last state is quit by the user
 * finger counts can be calculated by the median of the last frames
 * direction of linear movements can be limited by a maximum angle difference
 * joint relations and linear movements can be defined without directly using the coordinate axis, but by more intuitive types as "above, below, .." and "up, down, .."
 
Changes in 0.7.0
================
-Enhanced the Add..Recognizer(..) functions to have the same functionality as available in XML.
-Added new face tracking rendering
-Fixed bugs for maxAngleDiff-property, clearRecognizers()-function, OpenCV-preprocessor, face joints names, the updateUsers()-function, the updateTrackingData()-function and some more minor ones ..
-Removed some remaing Windows-specific parts -> Fubi has now already been succesfully compiled under Linux and Mac (CMake files or similar will probably follow in one of the next releases)
-Local positions now have the complete torso transformation (translation and rotation) removed, so they might be useful in cases where we want to look at directions only relative to the body (not the world coordinate system)
-Added body measurements (also renderable) usable for joint relation recognizers as alternative to concrete milimeters
-All printfs now are replaced by advanced logging functions that can also be deactivated according to the logging level (set in the FubiConfig.h)
-OpenNI2 sensor also approximates face positions for chin/forehead/nose/ears to make those more usable (Note: they are dependent on the torso orientation, but not on the head orientation [as this is not tracked by OpenNI at the moment], so they can give a good approximation in many cases, but not all the time).
-Combination states can now contain two other types of recognizers: NotRecognizers are fulfilled if the refered recognizer is NOT recognized; AlternativeRecognizers define a group of recognizers that are only tested if the regular ones have already failed ->  a way to combine the recognizers with OR instead of AND.
-Combinations with the isWaitingForLastStateFinish flag now return WAITING_FOR_LAST_STATE_TO_FINISH if last state has been fulfilled but not finished
-Several minor additions to the XML gesture definitions: noInterruptionBeforeMinDuration for Combinations and useOnlyCorrectDirectionComponent for linear movements.