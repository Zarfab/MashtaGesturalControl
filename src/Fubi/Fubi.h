// ****************************************************************************************
//
// Fubi API
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once
#ifndef FUBI_H
#define FUBI_H

#if defined ( WIN32 ) || defined( _WINDOWS )
#	 ifdef FUBI_EXPORTS
#       define FUBI_API __declspec( dllexport )
#	 else
#       define FUBI_API __declspec( dllimport )
#    endif
#else
#	 define FUBI_API
#endif


#include <set>
#include <vector>

#include "FubiPredefinedGestures.h"
#include "FubiUtils.h"
#include "FubiUser.h"

/**
 * \mainpage Fubi - Full Body Interaction Framework
 * 
 * \section intro_sec Introduction
 * 
 * Full Body Interaction Framework (FUBI) is a framework for recognizing full body gestures and postures in real time from the data of an OpenNI-applicable depth sensor, especially the Microsoft Kinect sensor.
  */


/**
 * \namespace Fubi
 *
 * \brief The Fubi namespace provides all methods to control the Full Body Interaction framework (FUBI).
 *
 */
namespace Fubi
{
	/** \addtogroup FUBI_API
	 * All the C++ API functions
	 * 
	 * @{
	 */

	/**
	 * \brief Initializes Fubi with OpenN 1.x using the given xml file and sets the skeleton profile.
	 *        If no xml file is given, Fubi will be intialized without OpenNI tracking enabled --> methods that need an openni context won't work.
	 * 
	 * @param openniXmlconfig name of the xml file for openNI initialization inlcuding all needed productions nodes 
		(should be placed in the working directory, i.e. "bin" folder)
		if config == 0x0, then OpenNI won't be initialized and Fubi stays in non-tracking mode
	 * @param profile set the openNI skeleton profile
	 * @param mirrorStream whether the stream should be mirrored or not
	 * @param smoothing set a smoothing factor for the tracking 0 = no smoothing to 0.99 strong smoothing
	 * @return true if succesfully initialized or already initialized before,
		false means bad xml file or other serious problem with OpenNI initialization
	 *
	 * Default openni xml configuration file (note that only specific resolutions and FPS values are allowed):
	 	<OpenNI>
		  <Log writeToConsole="true" writeToFile="false">
			<!-- 0 - Verbose, 1 - Info, 2 - Warning, 3 - Error (default) -->
			<LogLevel value="2"/>
		  </Log>
		  <ProductionNodes>
			<Node type="Image">
			  <Configuration>
				<MapOutputMode xRes="1280" yRes="1024" FPS="15"/>
				<Mirror on="true"/>
			  </Configuration>
			</Node>
			<Node type="Depth">
			  <Configuration>
				<MapOutputMode xRes="640" yRes="480" FPS="30"/>
				<Mirror on="true"/>
			  </Configuration>
			</Node>
			<Node type="Scene" />
			<Node type="User" />
		  </ProductionNodes>
		</OpenNI>
	 */
	FUBI_API bool init(const char* openniXmlconfig = 0x0, Fubi::SkeletonTrackingProfile::Profile profile = Fubi::SkeletonTrackingProfile::ALL,
		bool mirrorStream = true, float smoothing = 0);


	/**
	 * \brief Initializes Fubi with an options file for the sensor init
	 * 
	 * @param options configuration for the sensor
	 * @return true if succesfully initialized or already initialized before,
		false means problem with sensor init
		*/
	FUBI_API bool init(const SensorOptions& options);

	/**
	 * \brief Initializes Fubi with an specific options for the sensor init
	 * 
	 * @param depthWidth, ... options configuration for the sensor as in the SensorOptions struct
	 * @return true if succesfully initialized or already initialized before,
		false means problem with sensor init
		*/
	FUBI_API bool init(int depthWidth, int depthHeight, int depthFPS = 30,
		int rgbWidth = 640, int rgbHeight = 480, int rgbFPS = 30,
		int irWidth = -1, int irHeight = -1, int irFPS = -1,
		Fubi::SensorType::Type sensorType = Fubi::SensorType::OPENNI2,
		Fubi::SkeletonTrackingProfile::Profile profile = Fubi::SkeletonTrackingProfile::ALL,
		bool mirrorStream = true, float smoothing = 0);

	/**
	 * \brief Allows you to switch between different sensor types during runtime
	 *		  Note that this will also reinitialize most parts of Fubi
	 * 
	 * @param options options for initializing the new sensor
	 * @return true if the sensor has been succesfully initialized
		*/
	FUBI_API bool switchSensor(const SensorOptions& options);

	/**
	 * \brief Allows you to switch between different sensor types during runtime
	 *		  Note that this will also reinitialize most parts of Fubi
	 * 
	 * @param sensorType the sensor type to switch to
	 * @param depthWidth, ... options configuration for the sensor as in the SensorOptions struct
	 * @return true if the sensor has been succesfully initialized
		*/
	FUBI_API bool switchSensor(Fubi::SensorType::Type sensorType, int depthWidth, int depthHeight, int depthFPS = 30,
		int rgbWidth = 640, int rgbHeight = 480, int rgbFPS = 30,
		int irWidth = -1, int irHeight = -1, int irFPS = -1,
		Fubi::SkeletonTrackingProfile::Profile profile = Fubi::SkeletonTrackingProfile::ALL,
		bool mirrorStream = true, float smoothing = 0);


	/**
	 * \brief Get the currently available sensor types (defined in FubiConfig.h before compilation)
	 * 
	 * @return an int composed of the currently available sensor types (see SensorType enum for the meaning)
		*/
	FUBI_API int getAvailableSensorTypes();

	/**
	 * \brief Get the type of the currently active sensor
	 * 
	 * @return the current sensor type
		*/
	FUBI_API Fubi::SensorType::Type getCurrentSensorType();


	/**
	 * \brief Shuts down OpenNI and the tracker, releasing all allocated memory
	 * 
	 */
	FUBI_API void release();

	/**
	 * \brief Returns true if OpenNI has been already initialized
	 * 
	 */
	FUBI_API bool isInitialized();


	/**
	 * \brief Updates the sensor to get the next frame of depth, rgb, and tracking data.
	 *        Also searches for users in the scene and loads the default tracking calibration for new users or request a calibration
	 * 
	 */
	FUBI_API void updateSensor();

	/**
	 * \brief retrieve an image from one of the OpenNI production nodes with specific format and optionally enhanced by different
	 *        tracking information 
	 *		  Some render options require an OpenCV installation!
	 *
	 * @param outputImage pointer to a unsigned char array
	 *        Will be filled with wanted image
	 *		  Array has to be of correct size, e.g. depth image (640x480 std resolution) with tracking info
	 *		  requires 4 channels (RGBA) --> size = 640*480*4 = 1228800
	 * @param type can be color, depth, or ir image
	 * @param numChannels number channels in the image 1, 3 or 4
	 * @param depth the pixel depth of the image, 8 bit (standard) or 16 bit (mainly usefull for depth images
	 * @param renderOptions options for rendering additional informations into the image (e.g. tracking skeleton) or swapping the r and b channel
	 * @param depthModifications options for transforming the depth image to a more visible format
	 * @param userId If set to something else than 0 an image will be cut cropped around (the joint of interest of) this user, if 0 the whole image is put out.
	 * @param jointOfInterest the joint of the user the image is cropped around and a threshold on the depth values is applied.
			  If set to num_joints fubi tries to crop out the whole user.
	*/
	FUBI_API bool getImage(unsigned char* outputImage, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth, 
		unsigned int renderOptions = (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions),
		DepthImageModification::Modification depthModifications = DepthImageModification::UseHistogram,
		unsigned int userId = 0, Fubi::SkeletonJoint::Joint jointOfInterest = Fubi::SkeletonJoint::NUM_JOINTS);

	/**
	 * \brief save an image from one of the OpenNI production nodes with specific format and optionally enhanced by different
	 *        tracking information
	 *
	 * @param filename filename where the image should be saved to
	 *        can be relative to the working directory (bin folder) or absolute
	 *		  the file extension determins the file format (should be jpg)
	 * @param jpegQuality qualitiy (= 88) of the jpeg compression if a jpg file is requested, ranges from 0 to 100 (best quality)
	 * @param type can be color, depth, or ir image
	 * @param numChannels number channels in the image 1, 3 or 4
	 * @param depth the pixel depth of the image, 8 bit (standard) or 16 bit (mainly usefull for depth images
	 * @param renderOptions options for rendering additional informations into the image (e.g. tracking skeleton) or swapping the r and b channel
	 * @param depthModifications options for transforming the depht image to a more visible format
	 * @param userId If set to something else than 0 an image will be cut cropped around (the joint of interest of) this user, if 0 the whole image is put out.
	 * @param jointOfInterest the joint of the user the image is cropped around and a threshold on the depth values is applied.
			  If set to num_joints fubi tries to crop out the whole user.
	*/
	FUBI_API bool saveImage(const char* fileName, int jpegQuality, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth,
		unsigned int renderOptions = (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions),
		DepthImageModification::Modification depthModifications = DepthImageModification::UseHistogram,
		unsigned int userId = 0, Fubi::SkeletonJoint::Joint jointOfInterest = Fubi::SkeletonJoint::NUM_JOINTS);


	/**
	 * \brief Tries to recognize a posture in the current frame of tracking data of one user
	 * 
	 * @param postureID enum id of the posture to be found in FubiPredefinedGestures.h
	 * @param userID the OpenNI user id of the user to be checked
	 * @return RECOGNIZED in case of a succesful detection, TRACKING_ERROR if a needed joint is currently not tracked, NOT_RECOGNIZED else
	 */
	FUBI_API Fubi::RecognitionResult::Result recognizeGestureOn(Postures::Posture postureID, unsigned int userID);

	/**
	 * \brief Checks a user defined gesture or posture recognizer for its success
	 * 
	 * @param recognizerID id of the recognizer return during its creation
	 * @param userID the OpenNI user id of the user to be checked
	 * @return RECOGNIZED in case of a succesful detection, TRACKING_ERROR if a needed joint is currently not tracked, NOT_RECOGNIZED else
	 */
	FUBI_API Fubi::RecognitionResult::Result recognizeGestureOn(unsigned int recognizerIndex, unsigned int userID);

	/**
	 * \brief Checks a user defined gesture or posture recognizer for its success
	 * 
	 * @param recognizerName name of the recognizer return during its creation
	 * @param userID the OpenNI user id of the user to be checked
	 * @return RECOGNIZED in case of a succesful detection, TRACKING_ERROR if a needed joint is currently not tracked, NOT_RECOGNIZED else
	 */
	FUBI_API Fubi::RecognitionResult::Result recognizeGestureOn(const char* recognizerName, unsigned int userID);

	/**
	 * \brief Checks a combination recognizer for its progress
	 * 
	 * @param combinationID  enum id of the combination to be found in FubiPredefinedGestures.h
	 * @param userID the OpenNI user id of the user to be checked
	 * @param userStates (= 0x0) pointer to a vector of tracking data that represents the tracking information of the user
	 *		  during the recognition of each state
	 * @param restart (=true) if set to true, the recognizer automatically restarts, so the combination can be recognized again.
	 * @return RECOGNIZED in case of a succesful detection, TRACKING_ERROR if a needed joint is currently not tracked, NOT_RECOGNIZED else
	 */
	FUBI_API Fubi::RecognitionResult::Result getCombinationRecognitionProgressOn(Combinations::Combination combinationID, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates = 0x0, bool restart = true);

	/**
	 * \brief Checks a user defined combination recognizer for its progress
	 * 
	 * @param recongizerName name of the combination
	 * @param userID the OpenNI user id of the user to be checked
	 * @param userStates (= 0x0) pointer to a vector of tracking data that represents the tracking information of the user
	 *		  during the recognition of each state
	 * @param restart (=true) if set to true, the recognizer automatically restarts, so the combination can be recognized again.
	 * @return RECOGNIZED in case of a succesful detection, TRACKING_ERROR if a needed joint is currently not tracked, NOT_RECOGNIZED else
	 */
	FUBI_API Fubi::RecognitionResult::Result getCombinationRecognitionProgressOn(const char* recognizerName, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates = 0x0, bool restart = true);

	/**
	 * \brief Starts or stops the recognition process of a combination for one user
	 * 
	 * @param combinationID enum id of the combination to be found in FubiPredefinedGestures.h or Combinations::NUM_COMBINATIONS for all combinations
	 * @param userID the OpenNI user id of the user for whom the recognizers should be modified
	 * @param enable if set to true, the recognizer will be started (if not already stared), else it stops
	 */
	FUBI_API void enableCombinationRecognition(Combinations::Combination combinationID, unsigned int userID, bool enable);

	/**
	 * \brief Starts or stops the recognition process of a user defined combination for one user
	 * 
	 * @param combinationName name defined for this recognizer
	 * @param userID the OpenNI user id of the user for whom the recognizers should be modified
	 * @param enable if set to true, the recognizer will be started (if not already stared), else it stops
	 */
	FUBI_API void enableCombinationRecognition(const char* combinationName, unsigned int userID, bool enable);

	/**
	 * \brief Automatically starts combination recogntion for new users
	 * 
	 * @param enable if set to true, the recognizer will automatically start for new users, else this must be done manually (by using enableCombinationRecognition(..))
	 * @param combinationID enum id of the combination to be found in FubiPredefinedGestures.h or Combinations::NUM_COMBINATIONS for all combinations
	 */
	FUBI_API void setAutoStartCombinationRecognition(bool enable, Combinations::Combination combinationID = Combinations::NUM_COMBINATIONS);

	/**
	 * \brief Check if autostart is activated for a combination recognizer
	 * 
	 * @param combinationID enum id of the combination to be found in FubiPredefinedGestures.h orCombinations::NUM_COMBINATIONS for all combinations
	 * @return true if the corresponding auto start is activated
	 */
	FUBI_API bool getAutoStartCombinationRecognition(Combinations::Combination combinationID = Combinations::NUM_COMBINATIONS);


	/**
	 * \brief Returns the color for a user in the background image
	 * 
	 * @param id OpennNI user id of the user of interest
	 * @param r, g, b returns the red, green, and blue components of the color in which the users shape is displayed in the tracking image
	 */
	FUBI_API void getColorForUserID(unsigned int id, float& r, float& g, float& b);


	/**
	 * \brief Returns the OpenNI user id from the user index
	 * 
	 * @param index index of the user in the user array
	 * @return OpenNI user id of that user or 0 if not found
	 */
	FUBI_API unsigned int getUserID(unsigned int index);


	/**
	 * \brief Creates a user defined joint relation recognizer
	 * 
	 * @param joint the joint of interest
	 * @param relJoint the joint in which it has to be in a specific relation
	 * @param minValues (=-inf, -inf, -inf) the minimal values allowed for the vectore relJoint -> joint
	 * @param maxValues (=inf, inf, inf) the maximal values allowed for the vectore relJoint -> joint
	 * @param minDistance (= 0) the minimal distance between joint and relJoint
	 * @param maxDistance (= inf) the maximal distance between joint and relJoint
	 * @param useLocalPositions use positions in the local coordinate system of the user based on the torso transformation
	 * @param atIndex (= -1) if an index is given, the corresponding recognizer will be replaced instead of creating a new one
	 * @param name (= 0) sets a name for the recognizer (should be unique!)
	 * @param minConfidence (=-1) if given this is the mimimum confidence required from tracking for the recognition to be succesful
	 * @param measuringUnit the measuring unit for the values (millimeter by default)
	 *
	 * @return index of the recognizer needed to call it later
	 */
	FUBI_API unsigned int addJointRelationRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint,
		float minX = -Fubi::Math::MaxFloat, float minY = -Fubi::Math::MaxFloat, float minZ = -Fubi::Math::MaxFloat, 
		float maxX = Fubi::Math::MaxFloat, float maxY = Fubi::Math::MaxFloat, float maxZ = Fubi::Math::MaxFloat, 
		float minDistance = 0, 
		float maxDistance = Fubi::Math::MaxFloat,
		bool uselocalPositions = false,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence = -1.0f,
		Fubi::BodyMeasurement::Measurement measuringUnit = Fubi::BodyMeasurement::NUM_MEASUREMENTS);

	/**
	 * \brief Creates a user defined joint orientation recognizer
	 * 
	 * @param joint the joint of interest
	 * @param minValues (=-180, -180, -180) the minimal degrees allowed for the joint orientation
	 * @param maxValues (=180, 180, 180) the maximal degrees allowed for the joint orientation
	 * @param useLocalOrientations if true, uses a local orienation in which the parent orientation has been substracted
	 * @param atIndex (= -1) if an index is given, the corresponding recognizer will be replaced instead of creating a new one
	 * @param name (= 0) sets a name for the recognizer (should be unique!)
	 * @param minConfidence (=-1) if given this is the mimimum confidence required from tracking for the recognition to be succesful
	 *
	 * @return index of the recognizer needed to call it later
	 */
	FUBI_API unsigned int addJointOrientationRecognizer(Fubi::SkeletonJoint::Joint joint,
		float minX = -180.0f, float minY = -180.0f, float minZ = -180.0f,
		float maxX = 180.0f, float maxY = 180.0f, float maxZ = 180.0f,
		bool useLocalOrientations = true,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1);

	/**
	 * \brief Creates a user defined finger count recognizer
	 * 
	 * @param joint the hand joint of interest
	 * @param minFingers the minimum number of fingers the user should show up
	 * @param maxFingers the maximum number of fingers the user should show up
	 * @param atIndex (= -1) if an index is given, the corresponding recognizer will be replaced instead of creating a new one
	 * @param name (= 0) sets a name for the recognizer (should be unique!)
	 * @param minConfidence (=-1) if given this is the mimimum confidence required from tracking for the recognition to be succesful
	 * @param useMedianCalculation (=false) if true, the median for the finger count will be calculated over several frames instead of always taking the current detection
	 *
	 * @return index of the recognizer needed to call it later
	 */
	FUBI_API unsigned int addFingerCountRecognizer(Fubi::SkeletonJoint::Joint handJoint,
		unsigned int minFingers, unsigned int maxFingers,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1,
		bool useMedianCalculation = false);

	/**
	 * \brief Creates a user defined linear movement recognizer
	 * 
	 * @param joint the joint of interest
	 * @param relJoint the joint in which it has to be in a specifc relation
	 * @param direction the direction in which the movement should happen
	 * @param minVel the minimal velocity that has to be reached in this direction
	 * @param maxVel (= inf) the maximal velocity that is allowed in this direction
	 * @param atIndex (= -1) if an index is given, the corresponding recognizer will be replaced instead of creating a new one
	 * @param name name of the recognizer
	 * @param maxAngleDifference (=45�) the maximum angle difference that is allowed between the requested direction and the actual movement direction
	 * @param bool useOnlyCorrectDirectionComponent (=true) If true, this only takes the component of the actual movement that is conform
	 *				the requested direction, else it always uses the actual movement for speed calculation
	 *
	 * @return index of the recognizer needed to call it later
	 */
	// A linear gesture has a vector calculated as joint - relative joint, 
	// the direction (each component -1 to +1) that will be applied per component on the vector, and a min and max vel in milimeter per second
	FUBI_API unsigned int addLinearMovementRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint, 
		float dirX, float dirY, float dirZ, float minVel, float maxVel = Fubi::Math::MaxFloat,
		bool uselocalPositions = false,
		int atIndex = -1,
		const char* name = 0,
		float maxAngleDifference = 45.0f, 
		bool useOnlyCorrectDirectionComponent = true);
	FUBI_API unsigned int addLinearMovementRecognizer(SkeletonJoint::Joint joint, 
		float dirX, float dirY, float dirZ, float minVel, float maxVel = Fubi::Math::MaxFloat,
		bool uselocalPositions = false,
		int atIndex = -1,
		const char* name = 0,
		float maxAngleDifference = 45.0f, 
		bool useOnlyCorrectDirectionComponent = true);

	/**
	 * \brief load a combination recognizer from a string that represents an xml node with the combination definition
	 * 
	 * @para xmlDefinition string containing the xml definition
	 * @return true if the combination was loaded succesfully
	 */ 
	FUBI_API bool addCombinationRecognizer(const char* xmlDefinition);

	 /**
		 * \brief Loads a recognizer config xml file and adds the configured recognizers
		 * 
		 * @para fileName name of the xml config file
		 * @return true if at least one recognizers was loaded from the given xml
		 */ 
	 FUBI_API bool loadRecognizersFromXML(const char* fileName);

	/**
	 * \brief Returns current number of user defined recognizers
	 * 
	 * @return number of recognizers, the recognizers also have the indices 0 to numberOfRecs-1
	 */
	FUBI_API unsigned int getNumUserDefinedRecognizers();

	/**
	 * \brief Returns the name of a user defined recognizer
	 * 
	 * @param  recognizerIndex index of the recognizer
	 * @return returns the recognizer name or an empty string if the user is not found or the name not set
	 */
	FUBI_API const char* getUserDefinedRecognizerName(unsigned int recognizerIndex);

	/**
	 * \brief Returns the index of a user defined recognizer
	 * 
	 * @param recognizerName name of the recognizer
	 * @return returns the recognizer name or -1 if not found
	 */
	FUBI_API int getUserDefinedRecognizerIndex(const char* recognizerName);

	/**
	 * \brief Returns the index of a user defined combination recognizer
	 * 
	 * @param recognizerName name of the recognizer
	 * @return returns the recognizer name or -1 if not found
	 */
	FUBI_API int getUserDefinedCombinationRecognizerIndex(const char* recognizerName);

	/**
	 * \brief Returns current number of user defined combination recognizers
	 * 
	 * @return number of recognizers, the recognizers also have the indices 0 to numberOfRecs-1
	 */
	FUBI_API unsigned int getNumUserDefinedCombinationRecognizers();

	/**
	 * \brief Returns the name of a user defined combination recognizer
	 * 
	 * @param  recognizerIndex index of the recognizer
	 * @return returns the recognizer name or an empty string if the user is not found or the name not set
	 */
	FUBI_API const char* getUserDefinedCombinationRecognizerName(unsigned int recognizerIndex);

	/**
	 * \brief Returns all current users with their tracking information
	 * 
	 * @param pUserContainer (=0) pointer where a pointer to the current users will be stored at
	 *        The maximal size is Fubi::MaxUsers, but the current size can be requested by leaving the Pointer at 0
	 * @return the current number of users (= valid users in the container)
	 */
	FUBI_API unsigned short getCurrentUsers(FubiUser*** pUserContainer = 0);

	/**
	 * \brief Returns one user with his tracking information
	 * 
	 * @param id OpenNI id of the user
	 * @return a pointer to the user data
	 */
	FUBI_API FubiUser* getUser(unsigned int id);


	/**
	 * \brief Returns the current depth resolution or -1, -1 if failed
	 * 
	 * @param width, height the resolution
	 */
	FUBI_API void getDepthResolution(int& width, int& height);

	/**
	 * \brief Returns the current rgb resolution or -1, -1 if failed
	 * 
	 * @param width, height the resolution
	 */
	FUBI_API void getRgbResolution(int& width, int& height);

	/**
	 * \brief Returns the current ir resolution or -1, -1 if failed
	 * 
	 * @param width, height the resolution
	 */
	FUBI_API void getIRResolution(int& width, int& height);

	/**
	 * \brief Returns the number of shown fingers detected at the hand of one user (REQUIRES OPENCV!)
	 * 
	 * @param userID OpenNI id of the user
	 * @param leftHand looks at the left instead of the right hand
	 * @param getMedianOfLastFrames uses the precalculated median of finger counts of the last frames (still calculates new one if there is no precalculation)
	 * @param useOldConvexityDefectMethod if true using old method that calculates the convexity defects
	 * @return the number of shown fingers detected, 0 if there are none or there is an error
	 */
	FUBI_API int getFingerCount(unsigned int userID, bool leftHand = false, bool getMedianOfLastFrames = true, bool useOldConvexityDefectMethod = false);

	/**
	 * \brief Enables/Disables finger tracking for the hands of one user
	 *        If enabled the finger count will be tracked over time and the 
	 *		  median of these value will be returned in case of a query
	 *		  (REQUIRES OPENCV!)
	 * 
	 * @param userID OpenNI id of the user
	 * @param leftHand enable/disable finger tracking for the left hand
	 * @param rightHand enable/disable finger tracking for the right hand
	 */
	FUBI_API void enableFingerTracking(unsigned int userID, bool leftHand, bool rightHand, bool useConvexityDefectMethod = false);


	/**
	 * \brief  Whether the user is currently seen in the depth image
	 *
	 * @param userID OpenNI id of the user
	 */
	FUBI_API bool isUserInScene(unsigned int userID);

	/**
	 * \brief Whether the user is currently tracked
	 *
	 * @param userID OpenNI id of the user
	 */
	FUBI_API bool isUserTracked(unsigned int userID);
	

	/**
	 * \brief Get the most current tracking info of the user
	 * (including all joint positions and orientations (local and global) and a timestamp)
	 *
	 * @param userID OpenNI id of the user
	 * @return the user tracking info struct
	 */
	FUBI_API FubiUser::TrackingData* getCurrentTrackingData(unsigned int userId);

	/**
	 * \brief Get the last tracking info of the user (one frame before the current one)
	 * (including all joint positions and orientations (local and global) and a timestamp)
	 *
	 * @param userID OpenNI id of the user
	 * @return the user tracking info struct
	 */
	FUBI_API FubiUser::TrackingData* getLastTrackingData(unsigned int userId);
		
		
	/**
	 * \brief  Get the skeleton joint position out of the tracking info
	 *
	 * @param trackingInfo the trackinginfo struct to extract the info from
	 * @param jointId
	 * @param x, y, z [out] where the position of the joint will be copied to
	 * @param confidence [out] where the confidence for this position will be copied to
	 * @param timestamp [out] where the timestamp of this tracking info will be copied to (seconds since program start)
	 * @param localPosition if set to true, the function will return local position (vector from parent joint)
	 */
	FUBI_API void getSkeletonJointPosition(FubiUser::TrackingData* trackingInfo, SkeletonJoint::Joint joint, float& x, float& y, float& z, float& confidence, double& timeStamp, bool localPosition = false);

	/**
	 * \brief  Get the skeleton joint orientation out of the tracking info
	 *
	 * @param trackingInfo the trackinginfo struct to extract the info from
	 * @param jointId
	 * @param mat [out] rotation 3x3 matrix (9 floats)
	 * @param confidence [out] the confidence for this position
	 * @param timestamp [out] (seconds since program start)
	 * @param localOrientation if set to true, the function will local orientations (cleared of parent orientation) instead of globals
	 */
	FUBI_API void getSkeletonJointOrientation(FubiUser::TrackingData* trackingInfo, SkeletonJoint::Joint joint, float* mat, float confidence, double& timeStamp, bool localOrientation = true);

	/**
	 * \brief  Creates an empty vector of UserTrackinginfo structs
	 *
	 */
	FUBI_API std::vector<FubiUser::TrackingData>* createTrackingDataVector();
	
	/**
	 * \brief  Releases the formerly created vector
	 *
	 * @param vec the vector that will be released
	 */
	FUBI_API void releaseTrackingDataVector(std::vector<FubiUser::TrackingData>* vec);

	/**
	 * \brief  Returns the size of the vector
	 *
	 * @param vec the vector that we get the size of
	 */
	FUBI_API unsigned int getTrackingDataVectorSize(std::vector<FubiUser::TrackingData>* vec);
	/**
	 * \brief  Returns one element of the tracking info vector
	 *
	 * @param vec the vector that we get the element of
	 */
	FUBI_API FubiUser::TrackingData* getTrackingData(std::vector<FubiUser::TrackingData>* vec, unsigned int index);

	/**
	 * \brief Returns the OpenNI id of the user standing closest to the sensor (x-z plane)
	 */
	FUBI_API unsigned int getClosestUserID();

	/**
	 * \brief Returns the ids of all users order by their distance to the sensor (x-z plane)
	 * Closest user is at the front, user with largest distance or untracked users at the back
	 * If maxNumUsers is given, the given number of closest users is additionally ordered from left to right position
	 */
	FUBI_API std::deque<unsigned int> getClosestUserIDs(int maxNumUsers = -1);

	/**
	 * \brief Returns the user standing closest to the sensor (x-z plane)
	 */
	FUBI_API FubiUser* getClosestUser();

	/**
	 * \brief Returns all users order by their distance to the sensor (x-z plane)
	 * Closest user is at the front, user with largest distance or untracked users at the back
	 * If maxNumUsers is given, the given number of closest users is additionally ordered from left to right position
	 */
	FUBI_API std::deque<FubiUser*> getClosestUsers(int maxNumUsers = -1);

	/**
	 * \brief Stops and removes all user defined recognizers
	 */
	FUBI_API void clearUserDefinedRecognizers();

	/**
	 * \brief Set the current tracking info of one user
	 * (including all joint positions. Optionally the orientations and a timestamp)
	 *
	 * @param userID OpenNI id of the user
	 * @param positions an array of the joint positions
	 * @param timestamp the timestamp of the tracking value (if -1 an own timestamp will be created)
	 * @param orientations an array of the joint positions (if 0, the orientations will be approximated from the given positions)
	 * @param localOrientations same as orientations, but in the local coordinate system of the joint
	 */
	FUBI_API void updateTrackingData(unsigned int userId, Fubi::SkeletonJointPosition* positions,
		double timeStamp = -1, Fubi::SkeletonJointOrientation* orientations = 0, Fubi::SkeletonJointOrientation* localOrientations = 0);
	/* same function as before, but without OpenNI types,
	   i.e. 1 skeleton = per joint position, orientation, local orientation all three with 4 floats (x,y,z,conf) in milimeters or degrees,
	   localOrientsValid: set to false if you don't provide that data, timeStamp in seconds or -1 for self calculation*/
	FUBI_API void updateTrackingData(unsigned int userId, float* skeleton, bool localOrientsValid = true,
		double timeStamp = -1);

	/**
	 * \brief Calculate from real world coordinates (milimeters) to screen coordinates (pixels in the depth/rgb/ir image).
	 * Uses the data of a present sensor, or alternatively calculates the projection according to given sensor values, or to standard values
	 *
	 * @param realWorldVec vector with real world coordinates (in milimeters)
	 * @param xRes x resolution of the screen (depth/rgb/ir image)
	 * @param yRes y resolution of the screen (depth/rgb/ir image)
	 * @param hFOV the sensors horizontal field of view 
	 * @param vFOV the sensors vertical field of view 
	 * @return vector with screen coordinates (pixels in the depth/rgb/ir image)
	 */
	FUBI_API Fubi::Vec3f realWorldToProjective(const Fubi::Vec3f& realWorldVec, int xRes = 640, int yRes = 480,
		double hFOV = 1.0144686707507438, double vFOV = 0.78980943449644714);

	/**
	 * \brief Calculate from real world coordinates (milimeters) to screen coordinates (pixels in the depth/rgb/ir image).
	 * Uses the data of a present sensor, or alternatively calculates the projection according to given sensor values, or to standard values
	 *
	 * @param realWorldX, Y, Z vector with real world coordinates (in milimeters)
	 * @param screenX, Y, Z vector with screen coordinates (pixels in the depth/rgb/ir image)
	 * @param xRes x resolution of the screen (depth/rgb/ir image)
	 * @param yRes y resolution of the screen (depth/rgb/ir image)
	 * @param hFOV the sensors horizontal field of view 
	 * @param vFOV the sensors vertical field of view 
	 */
	FUBI_API void realWorldToProjective(float realWorldX, float realWorldY, float realWorldZ, float& screenX, float& screenY, float& screenZ,
		int xRes = 640, int yRes = 480,	double hFOV = 1.0144686707507438, double vFOV = 0.78980943449644714);
//
    FUBI_API std::vector<std::pair<std::string, std::vector<Fubi::SkeletonJoint::Joint> > > getCombinations();
	FUBI_API std::vector<Fubi::SkeletonJoint::Joint> getComboJoints(std::string comboName);
//
	/**
	 * \brief resests the tracking of all users
	 */
	FUBI_API void resetTracking();

	/**
	 * \brief get time since program start in seconds
	 */
	FUBI_API double getCurrentTime();

	/*! @}*/
}

#endif
