// ****************************************************************************************
//
// Fubi FubiCore
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once

// General includes
#include "Fubi.h"
#include "FubiUtils.h"
#include "FubiUser.h"
#include "FubiISensor.h"

// Recognizer interfaces
#include "GestureRecognizer/IGestureRecognizer.h"
#include "GestureRecognizer/CombinationRecognizer.h"

// STL containers
#include <map>
#include <vector>
#include <string>
#include <set>

// XML parsing
#include "rapidxml.hpp"

class FubiCore
{
public:
	// Singleton init only if not yet done
	static bool init(const char* xmlPath, Fubi::SkeletonTrackingProfile::Profile profile = Fubi::SkeletonTrackingProfile::ALL,
		bool mirrorStream = true, float smoothing = 0)
	{
		bool succes = true;
		if (s_instance == 0x0)
		{
			s_instance = new FubiCore();
			if (xmlPath == 0x0)
			{
				Fubi_logInfo("FubiCore: Initialized in non-tracking mode!\n");
			}
			else
			{
				// init with xml file
				succes = s_instance->initFromXml(xmlPath, profile, mirrorStream, smoothing);
			}
			
			if (!succes)
			{
				Fubi_logErr("Failed to inialize the sensor via XML!\n");
				delete s_instance;
				s_instance = 0x0;
			}
			else
			{
				Fubi_logInfo("FubiCore: Succesfully inialized the sensor via XML.\n");
			}
		}
		else
		{
			Fubi_logWrn("Fubi already initalized. New init will be ignored!\n");
		}
		return succes;
	}
	static bool init(const Fubi::SensorOptions& options)
	{
		bool succes = true;
		if (s_instance == 0x0)
		{
			s_instance = new FubiCore();

			// init with options
			succes = s_instance->initSensorWithOptions(options);
			
			if (!succes)
			{
				Fubi_logErr("Failed to inialize the sensor with the given options!\n");
				delete s_instance;
				s_instance = 0x0;
			}
			else
			{
				Fubi_logInfo("FubiCore: Succesfully inialized the sensor with the given options.\n");
			}
		}
		else
		{
			Fubi_logWrn("Fubi already initalized. New init will be ignored!\n");
		}
		return succes;
	}

	// Singleton getter (maybe null if not initialized!)
	static FubiCore* getInstance()
	{
		return s_instance;
	}

	// Release the singleton
	static void release()
	{
		delete s_instance;
		s_instance = 0x0;
	}

	void updateSensor();

	// Get the floor plane
	Fubi::Plane getFloor();

	// Get current users as an array
	unsigned short getCurrentUsers(FubiUser*** userContainer);

	void getDepthResolution(int& width, int& height);
	void getRgbResolution(int& width, int& height);
	void getIRResolution(int& width, int& height);

	// Add user defined gestures/postures
	unsigned int addJointRelationRecognizer(Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJoint::Joint relJoint,
		const Fubi::Vec3f& minValues = Fubi::Vec3f(-Fubi::Math::MaxFloat,-Fubi::Math::MaxFloat, -Fubi::Math::MaxFloat), 
		const Fubi::Vec3f& maxValues = Fubi::Vec3f(Fubi::Math::MaxFloat, Fubi::Math::MaxFloat, Fubi::Math::MaxFloat), 
		float minDistance = 0, 
		float maxDistance = Fubi::Math::MaxFloat,
		bool useLocalPositions = false,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1,
		Fubi::BodyMeasurement::Measurement measuringUnit = Fubi::BodyMeasurement::NUM_MEASUREMENTS);
	unsigned int addJointOrientationRecognizer(Fubi::SkeletonJoint::Joint joint,
		const Fubi::Vec3f& minValues = Fubi::Vec3f(-180.0f, -180.0f, -180.0f), const Fubi::Vec3f& maxValues = Fubi::Vec3f(180.0f, 180.0f, 180.0f),
		bool useLocalOrientations = true,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1);
	unsigned int addLinearMovementRecognizer(Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJoint::Joint relJoint, 
		const Fubi::Vec3f& direction, float minVel, float maxVel = Fubi::Math::MaxFloat, 
		bool useLocalPositions = false,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1,
		float maxAngleDiff = 45.0f, 
		bool useOnlyCorrectDirectionComponent = true);
	unsigned int addLinearMovementRecognizer(Fubi::SkeletonJoint::Joint joint,	const Fubi::Vec3f& direction, float minVel, float maxVel = Fubi::Math::MaxFloat, 
		bool useLocalPositions = false,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1,
		float maxAngleDiff = 45.0f, 
		bool useOnlyCorrectDirectionComponent = true);
	unsigned int addFingerCountRecognizer(Fubi::SkeletonJoint::Joint handJoint,
		unsigned int minFingers, unsigned int maxFingers,
		int atIndex = -1,
		const char* name = 0,
		float minConfidence =-1,
		bool useMedianCalculation = false);

	// load a combination recognizer from a string that represents an xml node with the combination definition
	bool addCombinationRecognizer(const std::string& xmlDefinition);

	// Load recognizers out of an xml configuration file
	bool loadRecognizersFromXML(const std::string& fileName);

	// Stop and remove all user defined recognizers
	void clearUserDefinedRecognizers();

	// Check current progress in gesture/posture recognition
	Fubi::RecognitionResult::Result recognizeGestureOn(Fubi::Postures::Posture postureID, unsigned int userID);
	Fubi::RecognitionResult::Result recognizeGestureOn(unsigned int recognizerIndex, unsigned int userID);
	Fubi::RecognitionResult::Result recognizeGestureOn(const std::string& recognizerName, unsigned int userID);
	Fubi::RecognitionResult::Result getCombinationRecognitionProgressOn(Fubi::Combinations::Combination combinationID, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates, bool restart);
	Fubi::RecognitionResult::Result getCombinationRecognitionProgressOn(const std::string& recognizerName, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates, bool restart);

	// Enable a posture combination recognition manually
	void enableCombinationRecognition(Fubi::Combinations::Combination combinationID, unsigned int userID, bool enable);
	// Enable a user defined posture combination recognition manually
	void enableCombinationRecognition(const std::string& combinationName, unsigned int userID, bool enable);
	// Or auto activate all for each new user
	void setAutoStartCombinationRecognition(bool enable, Fubi::Combinations::Combination combinationID = Fubi::Combinations::NUM_COMBINATIONS);
	bool getAutoStartCombinationRecognition(Fubi::Combinations::Combination combinationID = Fubi::Combinations::NUM_COMBINATIONS);

	// Get number of user defined recognizers
	unsigned int getNumUserDefinedRecognizers() { return m_userDefinedRecognizers.size(); }
	// Get given name of a recognizer or an empty string in case of failure
	const std::string& getUserDefinedRecognizerName(unsigned int index) { return (index < m_userDefinedRecognizers.size()) ? m_userDefinedRecognizers[index].first : s_emtpyString; }
	// Get index of a recognizer with the given name or -1 in case of failure
	int getUserDefinedRecognizerIndex(const std::string& name);
	// Get index of a recognizer with the given name or -1 in case of failure
	int getHiddenUserDefinedRecognizerIndex(const std::string& name);

	// Get number of user defined recognizers
	unsigned int getNumUserDefinedCombinationRecognizers() { return m_userDefinedCombinationRecognizers.size(); }
	// Get given name of a recognizer or an empty string in case of failure
	const std::string& getUserDefinedCombinationRecognizerName(unsigned int index) { return (index < m_userDefinedCombinationRecognizers.size()) ? m_userDefinedCombinationRecognizers[index].first : s_emtpyString; }
	// Get index of a recognizer with the given name or -1 in case of failure
	int getUserDefinedCombinationRecognizerIndex(const std::string& name);
	CombinationRecognizer* getUserDefinedCombinationRecognizer(const std::string& name);
	CombinationRecognizer* getUserDefinedCombinationRecognizer(unsigned int index);

	// Get the id (starting with 1) of a user by its index (starting with 0). Returns 0 if not found
	unsigned int getUserID(unsigned int index)
	{
		if (index < m_numUsers)
			return m_users[index]->m_id;
		return 0;
	}

	unsigned int getNumUsers() { return m_numUsers; }

	// Get user by id
	FubiUser* getUser(unsigned int userId);

	// Get the user standing closest to the sensor
	FubiUser* getClosestUser();
	unsigned int getClosestUserID();
	std::deque<unsigned int> getClosestUserIDs(int maxNumUsers = -1);
	std::deque<FubiUser*> getClosestUsers(int maxNumUsers = -1);


	/**
	 * \brief Set the current tracking info of one user
	 * (including all joint positions and the center of mass. Optionally the orientations and a timestamp)
	 *
	 * @param userID OpenNI id of the user
	 * @param positions an array of the joint positions
	 * @param com the center of mass
	 * @param timestamp the timestamp of the tracking value (if -1 an own timestamp will be created)
	 * @param orientations an array of the joint positions (if 0, the orientations will be approximated from the given positions)
	 * @param localOrientations same as orientations, but in the local coordinate system of the joint
	 */
	void updateTrackingData(unsigned int userId, Fubi::SkeletonJointPosition* positions, 
		double timeStamp = -1, Fubi::SkeletonJointOrientation* orientations = 0, Fubi::SkeletonJointOrientation* localOrientations = 0);
	/* same function as before, but without Fubi skeleton types,
	   i.e. 1 skeleton = per joint position, orientation, local orientation all three with 4 floats (x,y,z,conf) in milimeters or degrees,
	   localOrientsValid: set to false if you don't provide that data, timeStamp in seconds or -1 for self calculation*/
	void updateTrackingData(unsigned int userId, float* skeleton, bool localOrientsValid = true, double timeStamp = -1);

	// Return realworld to projective according to current sensor, or approximate it according to standard Kinect/Xtion values (if no sensor present)
	Fubi::Vec3f realWorldToProjective(const Fubi::Vec3f& realWorldVec, int xRes = 640, int yRes = 480,
		double hFOV = 1.0144686707507438, double vFOV = 0.78980943449644714);

	// Reset the tracking of all users in the current sensor
	void resetTracking();

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
	 * @param renderOptions options for rendering additional informations into the image (e.g. tracking skeleton) or swaping r and b channel
	 * @param depthModifications options for transforming the depht image to a more visible format
	 * @param userId If set to something else than 0 an image will be cut cropped around (the joint of interest of) this user, if 0 the whole image is put out.
	 * @param jointOfInterest the joint of the user the image is cropped around and a threshold on the depth values is applied.
			  If set to num_joints fubi tries to crop out the whole user.
	*/
	bool getImage(unsigned char* outputImage, Fubi::ImageType::Type type, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, 
		unsigned int renderOptions = (Fubi::RenderOptions::Shapes | Fubi::RenderOptions::Skeletons | Fubi::RenderOptions::UserCaptions),
		Fubi::DepthImageModification::Modification depthModifications = Fubi::DepthImageModification::UseHistogram,
        unsigned int userId = 0, Fubi::SkeletonJoint::Joint jointOfInterest = Fubi::SkeletonJoint::NUM_JOINTS);

    /**
     * \brief set the current gesture to be displayed on the image when calling getImage
     *
     * @param gesture the gesture name
     * @param userId
    */
    void setCurrentGesture(std::string gesture, unsigned int userId = 0);

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
	 * @param renderOptions options for rendering additional informations into the image (e.g. tracking skeleton) or swaping r and b channel
	 * @param depthModifications options for transforming the depht image to a more visible format
	 * @param userId If set to something else than 0 an image will be cut cropped around the joint of intereset of this user, if 0 the whole image is put out
	 * @param jointOfInterest the joint of the user the image is cropped around
	*/
	bool saveImage(const char* fileName, int jpegQuality, Fubi::ImageType::Type type, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth,
		unsigned int renderOptions = (Fubi::RenderOptions::Shapes | Fubi::RenderOptions::Skeletons | Fubi::RenderOptions::UserCaptions),
		Fubi::DepthImageModification::Modification depthModifications = Fubi::DepthImageModification::UseHistogram,
		unsigned int userId = 0, Fubi::SkeletonJoint::Joint jointOfInterest = Fubi::SkeletonJoint::NUM_JOINTS);

	// Get color of a user in the enhanced depth image
	static void getColorForUserID(unsigned int id, float& r, float& g, float& b);

	FubiISensor* getSensor() { return m_sensor; }

	// initialize sensro with an options file
	bool initSensorWithOptions(const Fubi::SensorOptions& options);
//
	void combinationRecToJoints();
	bool m_combinationSorted;
    std::vector<std::pair<std::string, std::vector<Fubi::SkeletonJoint::Joint> > > m_jointsCombinations;
//

private:
	// private constructor/destructor as it is a singeleton
	FubiCore();
	~FubiCore();

	// initialize with an xml file
	bool initFromXml(const char* xmlPath, Fubi::SkeletonTrackingProfile::Profile profile = Fubi::SkeletonTrackingProfile::ALL,
		bool mirrorStream = true, float smoothing = 0);

	// Update FubiUser -> OpenNI ID mapping 
	void updateUsers();

	// Load a combination recognizer from the given xml node
	bool loadCombinationRecognizerFromXML(rapidxml::xml_node<>* node, float globalMinConfidence);

	// The singleton instance of the tracker
	static FubiCore* s_instance;

	const static std::string s_emtpyString;

	// Number of current users
	unsigned short m_numUsers;
	// All users
	FubiUser* m_users[Fubi::MaxUsers];
	// Mapping of user ids to users
	std::map<unsigned int, FubiUser*> m_userIDToUsers;

	// One posture recognizer per posture
	IGestureRecognizer* m_postureRecognizers[Fubi::Postures::NUM_POSTURES];

	// User defined recognizers (joint relations and linear gestures) stored with name
	std::vector<std::pair<std::string, IGestureRecognizer*> > m_userDefinedRecognizers;
	// Hidden user defined recognizers (joint relations and linear gestures) stored with name,
	// can only be used in Combinations, but not directly
	std::vector<std::pair<std::string, IGestureRecognizer*> > m_hiddenUserDefinedRecognizers;
	// User defined Combination recognizers (templates to apply for each user)
	std::vector<std::pair<std::string, CombinationRecognizer*> > m_userDefinedCombinationRecognizers;
//
    std::vector<std::pair<std::string, std::vector<Fubi::SkeletonJoint::Joint> > > m_jointsRecognizers;
    std::vector<std::pair<std::string, std::string> > m_combinationRecognizers;
	void printLoadedRecognizers();
	void printLoadedCombinations();
	void printLoadedJointsCombinations();
	int findInStringVector(std::vector<std::string> vec, std::string s);
	int findInJointVector(std::vector<Fubi::SkeletonJoint::Joint> vec, Fubi::SkeletonJoint::Joint j);
    int findInStringPairVector(std::vector<std::pair<std::string, std::string> > vec, std::string s1, std::string s2);
//
	// The Combination recognizers that should start automatically when a new user is detected
	bool m_autoStartCombinationRecognizers[Fubi::Combinations::NUM_COMBINATIONS+1];

	FubiISensor* m_sensor;
    FubiUserGesture m_current_gesture;
};
