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

#include "FubiCore.h"

// Defines for enabling/disabling OpenNI and OpenCV dependencies
#include "FubiConfig.h"

// Posture recognition
#include "FubiRecognizerFactory.h"

// Image processing
#include "FubiImageProcessing.h"

#ifdef USE_OPENNI2
// OpenNI v2.x integration
#include "FubiOpenNI2Sensor.h"
#endif
#ifdef USE_OPENNI1
// OpenNI v1.x integration
#include "FubiOpenNISensor.h"
#endif
#ifdef USE_KINECT_SDK
// Kinect SDK integration
#include "FubiKinectSDKSensor.h"
#endif

// File reading for Xml parsing
#include <fstream>

// Sorting and more
#include <algorithm>
#include <iostream>

using namespace Fubi;
using namespace std;

const std::string FubiCore::s_emtpyString;

FubiCore* FubiCore::s_instance = 0x0;

FubiCore::~FubiCore()
{
	for (unsigned int i = 0; i < Postures::NUM_POSTURES; ++i)
	{
		delete m_postureRecognizers[i];
		m_postureRecognizers[i] = 0x0;
	}
	clearUserDefinedRecognizers();

	for (unsigned int i = 0; i < MaxUsers; ++i)
	{
		delete m_users[i];
		m_users[i] = 0x0;
	}
	m_numUsers = 0;

	delete m_sensor;
}

FubiCore::FubiCore() : m_numUsers(0), m_sensor(0x0)
{

	for (unsigned int i = 0; i < MaxUsers; ++i)
	{
		m_users[i] = new FubiUser();
	}

	for (unsigned int i = 0; i < Combinations::NUM_COMBINATIONS+1; ++i)
	{
		m_autoStartCombinationRecognizers[i] = false;
	}

	// Init posture recognizers
	for (unsigned int i = 0; i < Postures::NUM_POSTURES; ++i)
	{
		m_postureRecognizers[i] = createRecognizer((Postures::Posture)i);
	}
	
	// Combinations not sorted yet
	m_combinationSorted = false;
}

bool FubiCore::initFromXml(const char* xmlPath, Fubi::SkeletonTrackingProfile::Profile profile /*= Fubi::SkeletonTrackingProfile::ALL*/,
	bool mirrorStream /*= true*/, float smoothing /*= 0*/)
{
	delete m_sensor;
#ifdef USE_OPENNI1
	m_sensor = new FubiOpenNISensor();

	return m_sensor->initFromXml(xmlPath, profile, mirrorStream, smoothing);
#else
	if (xmlPath != 0)
		Fubi_logErr("Tried to init OpenNI 1.x via XML, but no activated sensor type supports xml init.\n -Did you forget to uncomment the USE_OPENNI1 define in the FubiConfig.h?\n");
	m_sensor = 0x0;
	return (xmlPath == 0x0);
#endif
}

bool FubiCore::initSensorWithOptions(const Fubi::SensorOptions& options)
{
	delete m_sensor;
	m_sensor = 0x0;
	bool succes = false;

	for (unsigned int i = 0; i < MaxUsers; ++i)
	{
		m_users[i]->m_inScene = false;
		m_users[i]->m_id = 0;
		m_users[i]->m_isTracked = false;
	}
	m_numUsers = 0;
	m_userIDToUsers.clear();

	if (options.m_type == SensorType::OPENNI2)
	{
#ifdef USE_OPENNI2
		m_sensor = new FubiOpenNI2Sensor();
		succes = m_sensor->initWithOptions(options);
#else
		Fubi_logErr("Openni 2.x sensor is not activated\n -Did you forget to uncomment the USE_OPENNI2 define in the FubiConfig.h?\n");	
#endif
	}
	else if (options.m_type == SensorType::OPENNI1)
	{
#ifdef USE_OPENNI1
		m_sensor = new FubiOpenNISensor();
		succes = m_sensor->initWithOptions(options);
#else
		Fubi_logErr("Openni 1.x sensor is not activated\n -Did you forget to uncomment the USE_OPENNI1 define in the FubiConfig.h?\n");	
#endif
	}
	else if (options.m_type == SensorType::KINECTSDK)
	{
#ifdef USE_KINECT_SDK
		m_sensor = new FubiKinectSDKSensor();
		succes = m_sensor->initWithOptions(options);
#else
		Fubi_logErr("Kinect SDK sensor is not activated\n -Did you forget to uncomment the USE_OPENNIX/USE_KINECTSDK define in the FubiConfig.h?\n");	
#endif
	}
	else if (options.m_type == SensorType::NONE)
	{
		Fubi_logInfo("FubiCore: Current sensor deactivated, now in non-tracking mode!\n");
		succes = true;
	}

	if (!succes)
	{
		delete m_sensor;
		m_sensor = 0x0;
	}

	return succes;
}

void FubiCore::updateSensor()
{
	if (m_sensor)
	{
		m_sensor->update();

		// Get the current number and ids of users, adapt the useridTouser map
		// init new users and update tracking info
		updateUsers();
	}
}


Fubi::RecognitionResult::Result FubiCore::recognizeGestureOn(Postures::Posture postureID, unsigned int userID)
{
	if (postureID < Postures::NUM_POSTURES && m_postureRecognizers[postureID])
	{
		FubiUser* user = getUser(userID);
		if (user)
		{	
			// Found user
			return m_postureRecognizers[postureID]->recognizeOn(user);
		}
	}
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}

Fubi::RecognitionResult::Result FubiCore::recognizeGestureOn(unsigned int recognizerIndex, unsigned int userID)
{
	if (recognizerIndex < m_userDefinedRecognizers.size())
	{
		FubiUser* user = getUser(userID);
		if (user && user->m_isTracked && user->m_inScene)
		{
			// Found the user
			return m_userDefinedRecognizers[recognizerIndex].second->recognizeOn(user);
		}
	}
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}

Fubi::RecognitionResult::Result FubiCore::recognizeGestureOn(const string& name, unsigned int userID)
{
	int recognizerIndex = getUserDefinedRecognizerIndex(name);
	if (recognizerIndex >= 0)
		return recognizeGestureOn((unsigned) recognizerIndex, userID);
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}

int FubiCore::getUserDefinedRecognizerIndex(const std::string& name)
{
	if (name.length() > 0)
	{
		vector<pair<string, IGestureRecognizer*> >::iterator iter = m_userDefinedRecognizers.begin();
		vector<pair<string, IGestureRecognizer*> >::iterator end = m_userDefinedRecognizers.end();
		for (int i = 0; iter != end; ++iter, ++i)
		{
			if (name == iter->first)
				return i;
		}
	}
	return -1;
}

int FubiCore::getHiddenUserDefinedRecognizerIndex(const std::string& name)
{
	if (name.length() > 0)
	{
        vector<pair<string, IGestureRecognizer*> >::iterator iter = m_hiddenUserDefinedRecognizers.begin();
        vector<pair<string, IGestureRecognizer*> >::iterator end = m_hiddenUserDefinedRecognizers.end();
		for (int i = 0; iter != end; ++iter, ++i)
		{
			if (name == iter->first)
				return i;
		}
	}
	return -1;
}

int FubiCore::getUserDefinedCombinationRecognizerIndex(const std::string& name)
{
	if (name.length() > 0)
	{
        vector<pair<string, CombinationRecognizer*> >::iterator iter = m_userDefinedCombinationRecognizers.begin();
        vector<pair<string, CombinationRecognizer*> >::iterator end = m_userDefinedCombinationRecognizers.end();
		for (int i = 0; iter != end; ++iter, ++i)
		{
			if (name == iter->first)
				return i;
		}
	}
	return -1;
}

CombinationRecognizer* FubiCore::getUserDefinedCombinationRecognizer(unsigned int index)
{
	if (index < m_userDefinedCombinationRecognizers.size())
	{
		return m_userDefinedCombinationRecognizers[index].second;
	}
	return 0x0;
}

CombinationRecognizer* FubiCore::getUserDefinedCombinationRecognizer(const std::string& name)
{
	if (name.length() > 0)
	{
        vector<pair<string, CombinationRecognizer*> >::iterator iter = m_userDefinedCombinationRecognizers.begin();
        vector<pair<string, CombinationRecognizer*> >::iterator end = m_userDefinedCombinationRecognizers.end();
		for (int i = 0; iter != end; ++iter, ++i)
		{
			if (name == iter->first)
				return iter->second;
		}
	}
	return 0x0;
}

void FubiCore::enableCombinationRecognition(Combinations::Combination combinationID, unsigned int userID, bool enable)
{
	// Standard case: enable/disable a single recognizer
	if (combinationID < Combinations::NUM_COMBINATIONS)
	{
		FubiUser* user = getUser(userID);
		if (user)
		{	
			// Found user
			user->enableCombinationRecognition(combinationID, enable);
		}
	}
	// Special case: enable/disable all recognizers (even the user defined ones!)
	else if (combinationID == Combinations::NUM_COMBINATIONS)
	{
		FubiUser* user = getUser(userID);
		if (user)
		{
			for (unsigned int i = 0; i < Combinations::NUM_COMBINATIONS; ++i)
				user->enableCombinationRecognition((Combinations::Combination)i, enable);

            std::vector<std::pair<std::string, CombinationRecognizer*> >::iterator iter;
            std::vector<std::pair<std::string, CombinationRecognizer*> >::iterator end = m_userDefinedCombinationRecognizers.end();
			for (iter = m_userDefinedCombinationRecognizers.begin(); iter != end; ++iter)
			{
				user->enableCombinationRecognition(iter->second, enable);
			}
		}
	}
}

void FubiCore::enableCombinationRecognition(const std::string& combinationName, unsigned int userID, bool enable)
{
	FubiUser* user = getUser(userID);
	if (user)
	{	
		// Found user
		user->enableCombinationRecognition(getUserDefinedCombinationRecognizer(combinationName), enable);
	}
}

bool FubiCore::getAutoStartCombinationRecognition(Fubi::Combinations::Combination combinationID /*= Fubi::Combinations::NUM_COMBINATIONS*/)
{
	if (m_autoStartCombinationRecognizers[Fubi::Combinations::NUM_COMBINATIONS])
		return true;
	return m_autoStartCombinationRecognizers[combinationID];
}

void FubiCore::setAutoStartCombinationRecognition(bool enable, Combinations::Combination combinationID /*= Combinations::NUM_COMBINATIONS*/)
{
	if (combinationID < Combinations::NUM_COMBINATIONS)
	{
		m_autoStartCombinationRecognizers[combinationID] = enable;
		if (enable)
		{
			// Enable it for all current users
			for (unsigned int user = 0; user < m_numUsers; user++)
			{
				m_users[user]->enableCombinationRecognition(combinationID, true);
			}
		}
	}
	else if (combinationID == Combinations::NUM_COMBINATIONS)
	{
		for (unsigned int i = 0; i < Combinations::NUM_COMBINATIONS; ++i)
			setAutoStartCombinationRecognition(enable, (Combinations::Combination)i);

		m_autoStartCombinationRecognizers[Combinations::NUM_COMBINATIONS] = enable;
		if (enable)
		{
			// Enable user defined recognizers for all current users
			for (unsigned int user = 0; user < m_numUsers; user++)
			{
				std::vector<std::pair<std::string, CombinationRecognizer*> >::iterator iter;
				std::vector<std::pair<std::string, CombinationRecognizer*> >::iterator end = m_userDefinedCombinationRecognizers.end();
				for (iter = m_userDefinedCombinationRecognizers.begin(); iter != end; ++iter)
				{
					m_users[user]->enableCombinationRecognition(iter->second, true);
				}
			}
		}
	}
}

Fubi::RecognitionResult::Result FubiCore::getCombinationRecognitionProgressOn(Combinations::Combination combinationID, unsigned int userID, vector<FubiUser::TrackingData>* userStates, bool restart)
{
	if (combinationID <Combinations::NUM_COMBINATIONS)
	{
		FubiUser* user = getUser(userID);
		if (user)
		{	
			// Found user
			if(user->m_combinationRecognizers[combinationID])
				return user->m_combinationRecognizers[combinationID]->getRecognitionProgress(userStates, restart);
			return Fubi::RecognitionResult::NOT_RECOGNIZED;
		}
	}
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}

Fubi::RecognitionResult::Result FubiCore::getCombinationRecognitionProgressOn(const std::string& recognizerName, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates, bool restart)
{
	FubiUser* user = getUser(userID);
	if (user)
	{
		std::map<string, CombinationRecognizer*>::iterator rec = user->m_userDefinedCombinationRecognizers.find(recognizerName);
		// Found the user
		if (rec != user->m_userDefinedCombinationRecognizers.end() && rec->second)
			return rec->second->getRecognitionProgress(userStates, restart);
		return Fubi::RecognitionResult::NOT_RECOGNIZED;
	}
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}

FubiUser* FubiCore::getUser(unsigned int userId)
{
	map<unsigned int, FubiUser*>::const_iterator iter = m_userIDToUsers.find(userId);
	if (iter != m_userIDToUsers.end())
	{
		return iter->second;
	}
	return 0;
}

void FubiCore::updateUsers()
{
	static unsigned int userIDs[MaxUsers];

	if (m_sensor)
	{
		// Get current user ids
		m_numUsers = m_sensor->getUserIDs(userIDs);

		// First sort our user array according to the given id array
		for (unsigned int i = 0; i < m_numUsers; ++i)
		{
			unsigned int id = userIDs[i];
			FubiUser* user = m_users[i];

            if(!user->m_inScene || !user->m_isTracked){
                FubiUserGesture::iterator _current_gesture = m_current_gesture.find( user->m_id );
                if(_current_gesture != m_current_gesture.end()){
                    m_current_gesture.erase(_current_gesture);
                }
            }

			if (user->m_id != id) // user at the wrong place or still unknown
			{
				// Try to find the user with the correct id (can only be later in the array as we already have corrected the ones before)
				// or at least find a free slot (at the current place or again later in the array)
				unsigned int oldIndex = -1;
				unsigned int firstFreeIndex = -1;
				for (unsigned int j = i; j < MaxUsers; ++j)
				{
					unsigned int tempId = m_users[j]->m_id;
					if (tempId == id)
					{
						oldIndex = j;
						break;
					}
					if (firstFreeIndex == -1 && tempId == 0)
					{
						firstFreeIndex = j;
					}
				}
				if (oldIndex != -1)
				{
					// Found him, so swap him to here
					std::swap(m_users[i], m_users[oldIndex]);
					user = m_users[i];
				}
				else
				{
					// Not found so look what we can do with the one currently here...
					if (firstFreeIndex != -1)
					{
						// We have a free slot to which we can move the current user
						std::swap(m_users[i], m_users[firstFreeIndex]);
					}
					else if (getUser(m_users[i]->m_id) == m_users[i])
					{
						// old user still valid, but no free slot available
						// so we have to drop him
						// Therefore, we remove the old map entry
						m_userIDToUsers.erase(m_users[i]->m_id);
					}
					// We now must have a usable user slot at the current index
					user = m_users[i];
					// but we have to reset his old data
					user->reset();
					// but keep him in scene
					user->m_inScene = true;
					// set correct id
					user->m_id = id;
					// and set his map entry
					m_userIDToUsers[id] = user;
				}
			}

			// Now the user has to be in the correct slot and everything should be set correctly
			
			bool wasTracked = user->m_isTracked;
			// get the tracking data from the sensor
			user->updateTrackingData(m_sensor);

			if (!wasTracked && user->m_isTracked)
			{
				// User tracking has started for this one!
				// Autostart posture combination detection
				for (unsigned int k = 0; k <Combinations::NUM_COMBINATIONS; ++k)
				{
					user->enableCombinationRecognition((Combinations::Combination)k,  false);
					if (getAutoStartCombinationRecognition((Combinations::Combination)k))
						user->enableCombinationRecognition((Combinations::Combination)k,  true);
				}
				// Special treatment for user definded posture combinations
				for (unsigned int j = 0; j < getNumUserDefinedCombinationRecognizers(); ++j)
				{
					CombinationRecognizer* rec = getUserDefinedCombinationRecognizer(j);
					user->enableCombinationRecognition(rec, false);
					if (getAutoStartCombinationRecognition(Fubi::Combinations::NUM_COMBINATIONS))
						user->enableCombinationRecognition(rec, true);
				}
			}
		}

		// invalidate all users after the now corrected ones
		for (unsigned int i = m_numUsers; i < MaxUsers; ++i)
		{
			// invalid user -> reset
			m_users[i]->reset();
		}

		// Cleanup userID map (should normally never be necesarry, TODO: test this)
		map<unsigned int, FubiUser*>::iterator iter;
		for (iter = m_userIDToUsers.begin(); iter != m_userIDToUsers.end();)
		{
			if (iter->second->m_id == 0 || iter->second->m_id != iter->first)
				m_userIDToUsers.erase(iter++);
			else
				++iter;
		}
	}
}

unsigned int FubiCore::addJointRelationRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint,
	const Vec3f& minValues /*= Vec3f(-Math::MaxFloat,-Math::MaxFloat, -Math::MaxFloat)*/, 
	const Vec3f& maxValues /*= Vec3f(Math::MaxFloat, Math::MaxFloat, Math::MaxFloat)*/, 
	float minDistance /*= 0*/, 
	float maxDistance /*= Math::MaxFloat*/,
	bool useLocalPositions /*= false*/,
	int atIndex /*=  -1*/,
	const char* name /*= 0*/,
	float minConfidence /*=-1*/,
	Fubi::BodyMeasurement::Measurement measuringUnit /*= Fubi::BodyMeasurement::NUM_MEASUREMENTS*/)
{
	string sName;
	if (name != 0)
		sName = name;
	// Add recognizer
	if (atIndex < 0 || (unsigned)atIndex >= m_userDefinedRecognizers.size())
	{
		// As a new one at the end
		atIndex = m_userDefinedRecognizers.size();
		m_userDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(sName, createRecognizer(joint, relJoint, minValues, maxValues, minDistance, maxDistance, useLocalPositions, minConfidence, measuringUnit)));
	}
	else
	{
		// Replacing an old one
		delete m_userDefinedRecognizers[atIndex].second;
		m_userDefinedRecognizers[atIndex].first = sName;
		m_userDefinedRecognizers[atIndex].second = createRecognizer(joint, relJoint, minValues, maxValues, minDistance, maxDistance, useLocalPositions, minConfidence, measuringUnit);
	}
	// Return index
	return atIndex;
}

unsigned int FubiCore::addJointOrientationRecognizer(SkeletonJoint::Joint joint,
		const Fubi::Vec3f& minValues /*= Fubi::Vec3f(-180.0f, -180.0f, -180.0f)*/, const Fubi::Vec3f& maxValues /*= Fubi::Vec3f(180.0f, 180.0f, 180.0f)*/,
		bool useLocalOrientations /*= true*/,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float minConfidence /*=-1*/)
{
	string sName;
	if (name != 0)
		sName = name;
	// Add recognizer
	if (atIndex < 0 || (unsigned)atIndex >= m_userDefinedRecognizers.size())
	{
		// As a new one at the end
		atIndex = m_userDefinedRecognizers.size();
		m_userDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(sName, createRecognizer(joint, minValues, maxValues, useLocalOrientations, minConfidence)));
	}
	else
	{
		// Replacing an old one
		delete m_userDefinedRecognizers[atIndex].second;
		m_userDefinedRecognizers[atIndex].first = sName;
		m_userDefinedRecognizers[atIndex].second = createRecognizer(joint, minValues, maxValues, useLocalOrientations, minConfidence);
	}
	// Return index
	return atIndex;
}

unsigned int FubiCore::addLinearMovementRecognizer(SkeletonJoint::Joint joint,	const Fubi::Vec3f& direction, float minVel, float maxVel /*= Fubi::Math::MaxFloat*/, 
		bool useLocalPositions /*= false*/,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float minConfidence /*=-1*/,
		float maxAngleDiff /*= 45.0f*/, 
		bool useOnlyCorrectDirectionComponent /*= true*/)
{
	string sName;
	if (name != 0)
		sName = name;
	// Add recognizer
	if (atIndex < 0 || (unsigned)atIndex >= m_userDefinedRecognizers.size())
	{
		// As a new one at the end
		atIndex = m_userDefinedRecognizers.size();
		m_userDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(sName, createRecognizer(joint, direction, minVel, maxVel, useLocalPositions, minConfidence, maxAngleDiff, useOnlyCorrectDirectionComponent)));
	}
	else 
	{
		// Replacing an old one
		delete m_userDefinedRecognizers[atIndex].second;
		m_userDefinedRecognizers[atIndex].first = sName;
		m_userDefinedRecognizers[atIndex].second = createRecognizer(joint, direction, minVel, maxVel, useLocalPositions, minConfidence, maxAngleDiff, useOnlyCorrectDirectionComponent);
	}
	// Return index
	return atIndex;
}

unsigned int FubiCore::addLinearMovementRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint, 
	const Vec3f& direction, float minVel, float maxVel /*= Fubi::Math::MaxFloat*/,
	bool useLocalPositions /*= false*/,
	int atIndex /*=  -1*/, const char* name /*= 0*/,
	float minConfidence /*=-1*/,
	float maxAngleDiff /*= 45.0f*/, 
	bool useOnlyCorrectDirectionComponent /*= true*/)
{
	string sName;
	if (name != 0)
		sName = name;
	// Add recognizer
	if (atIndex < 0 || (unsigned)atIndex >= m_userDefinedRecognizers.size())
	{
		// As a new one at the end
		atIndex = m_userDefinedRecognizers.size();
		m_userDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(sName, createRecognizer(joint, relJoint, direction, minVel, maxVel, useLocalPositions, minConfidence, maxAngleDiff, useOnlyCorrectDirectionComponent)));
	}
	else 
	{
		// Replacing an old one
		delete m_userDefinedRecognizers[atIndex].second;
		m_userDefinedRecognizers[atIndex].first = sName;
		m_userDefinedRecognizers[atIndex].second = createRecognizer(joint, relJoint, direction, minVel, maxVel, useLocalPositions, minConfidence, maxAngleDiff, useOnlyCorrectDirectionComponent);
	}
	// Return index
	return atIndex;
}

unsigned int FubiCore::addFingerCountRecognizer(SkeletonJoint::Joint handJoint,
		unsigned int minFingers, unsigned int maxFingers,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float minConfidence /*=-1*/,
		bool useMedianCalculation /*= false*/)
{
	string sName;
	if (name != 0)
		sName = name;
	// Add recognizer
	if (atIndex < 0 || (unsigned)atIndex >= m_userDefinedRecognizers.size())
	{
		// As a new one at the end
		atIndex = m_userDefinedRecognizers.size();
		m_userDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(sName, createRecognizer(handJoint, minFingers, maxFingers, minConfidence, useMedianCalculation)));
	}
	else
	{
		// Replacing an old one
		delete m_userDefinedRecognizers[atIndex].second;
		m_userDefinedRecognizers[atIndex].first = sName;
		m_userDefinedRecognizers[atIndex].second = createRecognizer(handJoint, minFingers, maxFingers, minConfidence, useMedianCalculation);
	}
	// Return index
	return atIndex;
}

unsigned short FubiCore::getCurrentUsers(FubiUser*** userContainer)
{
	if (userContainer != 0)
	{
		*userContainer = m_users;
	}
	return m_numUsers;
}

bool FubiCore::addCombinationRecognizer(const std::string& xmlDefinition)
{
	bool loaded = false;

	// copy string to buffer
	char* buffer = new char [xmlDefinition.length()+1];
#pragma warning(push)
#pragma warning(disable:4996)
	strcpy(buffer, xmlDefinition.c_str());
#pragma warning(pop)

	// parse XML
	rapidxml::xml_document<> doc;
	doc.parse<0>(buffer);
	rapidxml::xml_node<>* node = doc.first_node("CombinationRecognizer");
	if (node)
	{
		loaded = loadCombinationRecognizerFromXML(node, -1);
	}

	// release xml doc and buffer
	doc.clear();
	delete[] buffer;

	return loaded;
}

bool FubiCore::loadRecognizersFromXML(const std::string& fileName)
{
	// Open the file and copy the data to a buffer
	fstream file;
	file.open (fileName.c_str(), fstream::in | fstream::binary );

	if (!file.is_open() || !file.good())
		return false;

	bool loadedAnything = false;

	// get length of file:
	file.seekg (0, fstream::end);
	int length = (int)file.tellg();
	file.seekg (0, fstream::beg);
	// allocate memory:
	char* buffer = new char [length+1];
	// read data as a block:
	file.read(buffer, length);
	// null terminate the string
	buffer[length] = '\0';
	// and close the file
	file.close();

	// Load the string to the parser
	rapidxml::xml_document<> doc;    // character type defaults to char
	doc.parse<0>(buffer);

	// Parse the content
	rapidxml::xml_node<>* node = doc.first_node("FubiRecognizers");
	if (node)
	{
		float globalMinConf = -1.0f;
		rapidxml::xml_attribute<>* globalMinConfA = node->first_attribute("globalMinConfidence");
		if (globalMinConfA)
			globalMinConf = (float)atof(globalMinConfA->value());
		rapidxml::xml_node<>* recNode;
		for(recNode = node->first_node("JointRelationRecognizer"); recNode; recNode = recNode->next_sibling("JointRelationRecognizer"))
		{
			std::string name;
			rapidxml::xml_attribute<>* attr = recNode->first_attribute("name");
			if (attr)
				name = attr->value();

			bool visible = true;
			attr = recNode->first_attribute("visibility");
			if (attr)
				visible = removeWhiteSpacesAndToLower(attr->value()) != "hidden";

			bool localPos = false;
			attr = recNode->first_attribute("useLocalPositions");
			if (attr)
				localPos = removeWhiteSpacesAndToLower(attr->value()) != "false";

			float minConf = globalMinConf;
			rapidxml::xml_attribute<>* minConfA = recNode->first_attribute("minConfidence");
			if (minConfA)
				minConf = (float)atof(minConfA->value());

			BodyMeasurement::Measurement measure = BodyMeasurement::NUM_MEASUREMENTS;
			rapidxml::xml_attribute<>* measuringUnit = recNode->first_attribute("measuringUnit");
			if (measuringUnit)
				measure = Fubi::getBodyMeasureID(measuringUnit->value());

			SkeletonJoint::Joint joint = SkeletonJoint::RIGHT_HAND;
			SkeletonJoint::Joint relJoint = SkeletonJoint::NUM_JOINTS;
			rapidxml::xml_node<>* jointNode = recNode->first_node("Joints");
			if (jointNode)
			{
//
				vector<SkeletonJoint::Joint> sj;
				attr = jointNode->first_attribute("main");
				if (attr)
				{
					joint = getJointID(attr->value());
					sj.push_back(joint);
				}
				attr = jointNode->first_attribute("relative");
				if (attr)
				{
					relJoint = getJointID(attr->value());
					sj.push_back(relJoint);
				}
                m_jointsRecognizers.push_back(pair<string, vector<SkeletonJoint::Joint> >(name, sj));
//
			}

			Vec3f minValues = DefaultMinVec;
			float minDistance = 0;
			rapidxml::xml_node<>* minNode = recNode->first_node("MinValues");
			if (minNode)
			{
				attr = minNode->first_attribute("x");
				if (attr)
					minValues.x = (float) atof(attr->value());
				attr = minNode->first_attribute("y");
				if (attr)
					minValues.y = (float) atof(attr->value());
				attr = minNode->first_attribute("z");
				if (attr)
					minValues.z = (float) atof(attr->value());

				attr = minNode->first_attribute("dist");
				if (attr)
					minDistance = (float) atof(attr->value());
			}

			Vec3f maxValues = DefaultMaxVec;
			float maxDistance = Math::MaxFloat;
			rapidxml::xml_node<>* maxNode = recNode->first_node("MaxValues");
			if (maxNode)
			{
				attr = maxNode->first_attribute("x");
				if (attr)
					maxValues.x = (float) atof(attr->value());
				attr = maxNode->first_attribute("y");
				if (attr)
					maxValues.y = (float) atof(attr->value());
				attr = maxNode->first_attribute("z");
				if (attr)
					maxValues.z = (float) atof(attr->value());

				attr = maxNode->first_attribute("dist");
				if (attr)
					maxDistance = (float) atof(attr->value());
			}


			for(rapidxml::xml_node<>* relNode = recNode->first_node("Relation"); relNode; relNode = relNode->next_sibling("Relation"))
			{
				float min = -Math::MaxFloat;
				float max = Math::MaxFloat;
				attr = relNode->first_attribute("min");
				if (attr)
					min = (float) atof(attr->value());
				attr = relNode->first_attribute("max");
				if (attr)
					max = (float) atof(attr->value());
				attr = relNode->first_attribute("type");
				if (attr)
				{
					std::string lowerValue = removeWhiteSpacesAndToLower(attr->value());
                    if (lowerValue == "infrontof")
					{
						maxValues.z = -min;
						minValues.z = -max;
					}
                    else if (lowerValue == "behind")
					{
						maxValues.z = max;
						minValues.z = min;
					}
                    else if (lowerValue == "leftof")
					{
						maxValues.x = -min;
						minValues.x = -max;
					}
                    else if (lowerValue == "rightof")
					{
						maxValues.x = max;
						minValues.x = min;
					}
                    else if (lowerValue == "above")
					{
						maxValues.y = max;
						minValues.y = min;
					}
                    else if (lowerValue == "below")
					{
						maxValues.y = -min;
						minValues.y = -max;
					}
                    else if (lowerValue == "apartof")
					{
						minDistance = min;
						maxDistance = max;
					}
				}

			}
			if (visible)
				addJointRelationRecognizer(joint, relJoint, 
					minValues, maxValues, minDistance, maxDistance,
					localPos, -1, name.c_str(), minConf, measure);
			else
				m_hiddenUserDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(name, createRecognizer(joint, relJoint, minValues, maxValues, minDistance, maxDistance, localPos, minConf, measure)));
			loadedAnything = true;
		}

		for(recNode = node->first_node("JointOrientationRecognizer"); recNode; recNode = recNode->next_sibling("JointOrientationRecognizer"))
		{
			std::string name;
			rapidxml::xml_attribute<>* attr = recNode->first_attribute("name");
			if (attr)
				name = attr->value();

			bool visible = true;
			attr = recNode->first_attribute("visibility");
			if (attr)
				visible = removeWhiteSpacesAndToLower(attr->value()) != "hidden";

			bool localRot = true;
			attr = recNode->first_attribute("useLocalOrientations");
			if (attr)
				localRot = removeWhiteSpacesAndToLower(attr->value()) != "false";

			float minConf = globalMinConf;
			rapidxml::xml_attribute<>* minConfA = recNode->first_attribute("minConfidence");
			if (minConfA)
				minConf = (float)atof(minConfA->value());

			SkeletonJoint::Joint joint = SkeletonJoint::TORSO;
			rapidxml::xml_node<>* jointNode = recNode->first_node("Joint");
			if (jointNode)
			{
//
				vector<SkeletonJoint::Joint> sj;
				attr = jointNode->first_attribute("name");
				if (attr)
				{
					joint = getJointID(attr->value());
					sj.push_back(joint);
				}
                m_jointsRecognizers.push_back(pair<string, vector<SkeletonJoint::Joint> >(name, sj));
//
			}

			Vec3f minValues = Vec3f(-180.0f, -180.0f, -180.0f);
			rapidxml::xml_node<>* minNode = recNode->first_node("MinDegrees");
			if (minNode)
			{
				attr = minNode->first_attribute("x");
				if (attr)
					minValues.x = (float) atof(attr->value());
				attr = minNode->first_attribute("y");
				if (attr)
					minValues.y = (float) atof(attr->value());
				attr = minNode->first_attribute("z");
				if (attr)
					minValues.z = (float) atof(attr->value());
			}

			Vec3f maxValues = Vec3f(180.0f, 180.0f, 180.0f);
			rapidxml::xml_node<>* maxNode = recNode->first_node("MaxDegrees");
			if (maxNode)
			{
				attr = maxNode->first_attribute("x");
				if (attr)
					maxValues.x = (float) atof(attr->value());
				attr = maxNode->first_attribute("y");
				if (attr)
					maxValues.y = (float) atof(attr->value());
				attr = maxNode->first_attribute("z");
				if (attr)
					maxValues.z = (float) atof(attr->value());
			}

			if (visible)
				addJointOrientationRecognizer(joint, minValues, maxValues, localRot, -1, name.c_str(), minConf);
			else
				m_hiddenUserDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(name, createRecognizer(joint, minValues, maxValues, localRot, minConf)));
			loadedAnything = true;
		}
		
		for(recNode = node->first_node("LinearMovementRecognizer"); recNode; recNode = recNode->next_sibling("LinearMovementRecognizer"))
		{
			std::string name;
			rapidxml::xml_attribute<>* attr = recNode->first_attribute("name");
			if (attr)
				name = attr->value();

			bool visible = true;
			attr = recNode->first_attribute("visibility");
			if (attr)
				visible = removeWhiteSpacesAndToLower(attr->value()) != "hidden";

			bool localPos = false;
			attr = recNode->first_attribute("useLocalPositions");
			if (attr)
				localPos = removeWhiteSpacesAndToLower(attr->value()) != "false";

			float minConf = globalMinConf;
			rapidxml::xml_attribute<>* minConfA = recNode->first_attribute("minConfidence");
			if (minConfA)
				minConf = (float)atof(minConfA->value());

			bool useOnlyCorrectDirectionComponent = true;
			attr = recNode->first_attribute("useOnlyCorrectDirectionComponent");
			if (attr)
				useOnlyCorrectDirectionComponent = removeWhiteSpacesAndToLower(attr->value()) != "false";

			SkeletonJoint::Joint joint = SkeletonJoint::RIGHT_HAND;
			SkeletonJoint::Joint relJoint = SkeletonJoint::NUM_JOINTS;
			bool useRelative = false;
			rapidxml::xml_node<>* jointNode = recNode->first_node("Joints");
			if (jointNode)
			{
//				
				vector<SkeletonJoint::Joint> sj;
				attr = jointNode->first_attribute("main");
				if (attr)
				{
					joint = getJointID(attr->value());
					sj.push_back(joint);
				}
				attr = jointNode->first_attribute("relative");
				if (attr)
				{
					relJoint = getJointID(attr->value());
					sj.push_back(relJoint);
					useRelative = true;
				}
                m_jointsRecognizers.push_back(pair<string, vector<SkeletonJoint::Joint> >(name, sj));
//
			}

			Vec3f direction;
			float minVel = 0;
			float maxVel = Math::MaxFloat;
			float maxAngleDiff = 45.0f;

			rapidxml::xml_node<>* dirNode = recNode->first_node("Direction");
			if (dirNode)
			{
				attr = dirNode->first_attribute("x");
				if (attr)
					direction.x = (float) atof(attr->value());
				attr = dirNode->first_attribute("y");
				if (attr)
					direction.y = (float) atof(attr->value());
				attr = dirNode->first_attribute("z");
				if (attr)
					direction.z = (float) atof(attr->value());
				attr = dirNode->first_attribute("maxAngleDifference");
				if (attr)
					maxAngleDiff = (float) atof(attr->value());
			}

			dirNode = recNode->first_node("BasicDirection");
			if (dirNode)
			{
				attr = dirNode->first_attribute("type");
				if (attr)
				{
					std::string lowerValue = removeWhiteSpacesAndToLower(attr->value());
                    if (lowerValue == "left")
					{
						direction = Vec3f(-1.0f, 0, 0);
					}
                    else if (lowerValue == "right")
					{
						direction = Vec3f(1.0f, 0, 0);
					}
                    else if (lowerValue == "up")
					{
						direction = Vec3f(0, 1.0f, 0);
					}
                    else if (lowerValue == "down")
					{
						direction = Vec3f(0, -1.0f, 0);
					}
                    else if (lowerValue == "forward")
					{
						direction = Vec3f(0, 0, -1.0f);
					}
                    else if (lowerValue == "backward")
					{
						direction = Vec3f(0, 0, 1.0f);
					}
                    else if (lowerValue == "anydirection")
					{
						direction = Vec3f(0, 0, 0);
					}
				}
				attr = dirNode->first_attribute("maxAngleDifference");
				if (attr)
					maxAngleDiff = (float) atof(attr->value());
			}

			rapidxml::xml_node<>* speedNode = recNode->first_node("Speed");
			if (speedNode)
			{
				attr = speedNode->first_attribute("min");
				if (attr)
					minVel = (float) atof(attr->value());
				attr = speedNode->first_attribute("max");
				if (attr)
					maxVel = (float) atof(attr->value());
			}

			if (useRelative)
			{
				if (visible)
					addLinearMovementRecognizer(joint, relJoint, direction, minVel, maxVel, localPos, -1, name.c_str(), minConf, maxAngleDiff, useOnlyCorrectDirectionComponent);
				else
					m_hiddenUserDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(name, createRecognizer(joint, relJoint, direction, minVel, maxVel, localPos, minConf, maxAngleDiff, useOnlyCorrectDirectionComponent)));
			}
			else
			{
				if (visible)
					addLinearMovementRecognizer(joint, 
						direction, minVel, maxVel,
						localPos,
						-1, name.c_str(), minConf, maxAngleDiff, useOnlyCorrectDirectionComponent);
				else
					m_hiddenUserDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(name, createRecognizer(joint, direction, minVel, maxVel, localPos, minConf, maxAngleDiff, useOnlyCorrectDirectionComponent)));
			}
			loadedAnything = true;
		}

		for(recNode = node->first_node("FingerCountRecognizer"); recNode; recNode = recNode->next_sibling("FingerCountRecognizer"))
		{
			std::string name;
			rapidxml::xml_attribute<>* attr = recNode->first_attribute("name");
			if (attr)
				name = attr->value();

			bool visible = true;
			attr = recNode->first_attribute("visibility");
			if (attr)
				visible = removeWhiteSpacesAndToLower(attr->value()) != "hidden";

			float minConf = globalMinConf;
			rapidxml::xml_attribute<>* minConfA = recNode->first_attribute("minConfidence");
			if (minConfA)
				minConf = (float)atof(minConfA->value());

			
			SkeletonJoint::Joint joint = SkeletonJoint::RIGHT_HAND;
			rapidxml::xml_node<>* jointNode = recNode->first_node("Joint");
			if (jointNode)
			{
//				
				vector<SkeletonJoint::Joint> sj;
				attr = jointNode->first_attribute("name");
				if (attr)
				{
					joint = getJointID(attr->value());
					sj.push_back(joint);
				}
                m_jointsRecognizers.push_back(pair<string, vector<SkeletonJoint::Joint> >(name, sj));
//
			}

			unsigned int minFingers = 0;
			unsigned int maxFingers = 5;
			bool useMedian = false;
			rapidxml::xml_node<>* countNode = recNode->first_node("FingerCount");
			if (countNode)
			{
				attr = countNode->first_attribute("min");
				if (attr)
					minFingers = (unsigned)atoi(attr->value());
				attr = countNode->first_attribute("max");
				if (attr)
					maxFingers = (unsigned)atoi(attr->value());
				attr = countNode->first_attribute("useMedianCalculation");
				if (attr)
				{
					std::string lowerValue = removeWhiteSpacesAndToLower(attr->value());
					useMedian = lowerValue != "0" && lowerValue != "false";
				}
			}

			if (visible)
				addFingerCountRecognizer(joint, minFingers, maxFingers, -1, name.c_str(), minConf, useMedian);
			else
				m_hiddenUserDefinedRecognizers.push_back(pair<string, IGestureRecognizer*>(name, createRecognizer(joint, minFingers, maxFingers, minConf, useMedian)));
			loadedAnything = true;
		}

		for(recNode = node->first_node("CombinationRecognizer"); recNode; recNode = recNode->next_sibling("CombinationRecognizer"))
		{
			if (loadCombinationRecognizerFromXML(recNode, globalMinConf))
				loadedAnything = true;
		}

		bool oldCombinations = false;
		for(recNode = node->first_node("PostureCombinationRecognizer"); recNode; recNode = recNode->next_sibling("PostureCombinationRecognizer"))
		{
			if (loadCombinationRecognizerFromXML(recNode, globalMinConf))
			{
				loadedAnything = true;
				oldCombinations = true;
			}
		}
		if (oldCombinations)
		{
			Fubi_logWrn("XML_Warning - \"PostureCombinationRecognizer\" deprecated, please use \"CombinationRecognizer\"!\n");
		}

	}

	doc.clear();
	// release the buffer
	delete[] buffer;
//
	/*printLoadedRecognizers();
	cout << endl;
	printLoadedCombinations();
	cout << endl;*/
//
	return loadedAnything;
}

bool FubiCore::loadCombinationRecognizerFromXML(rapidxml::xml_node<>* node, float globalMinConfidence)
{
	bool succes = false;

	std::string name;
	rapidxml::xml_attribute<>* attr = node->first_attribute("name");
	if (attr)
	{
		name = attr->value();

		// Create posture combination recognizer template (not assigned to a user)
		CombinationRecognizer* rec = new CombinationRecognizer(name);

		attr = node->first_attribute("waitUntilLastStateRecognizersStop");
		if (attr)
		{
			std::string lowerValue = removeWhiteSpacesAndToLower(attr->value());
			rec->setWaitUntilLastStateRecognizersStop(lowerValue != "0" && lowerValue != "false");
		}

		rapidxml::xml_node<>* stateNode;
		int stateNum;
		for(stateNode = node->first_node("State"), stateNum = 1; stateNode; stateNode = stateNode->next_sibling("State"), stateNum++)
		{
			double maxDuration = -1;
			double minDuration = 0;
			double timeForTransition = 1.0;
			double maxInterruption = -1;

			attr = stateNode->first_attribute("maxDuration");
			if (attr)
				maxDuration = atof(attr->value());
			attr = stateNode->first_attribute("minDuration");
			if (attr)
				minDuration = atof(attr->value());
			attr = stateNode->first_attribute("timeForTransition");
			if (attr)
				timeForTransition = atof(attr->value());
			attr = stateNode->first_attribute("maxInterruptionTime");
			if (attr)
				maxInterruption = atof(attr->value());
			bool noInterrruptionBeforeMinDuration = false;
			attr = stateNode->first_attribute("noInterrruptionBeforeMinDuration");
			if (attr)
			{
				std::string lowerValue = removeWhiteSpacesAndToLower(attr->value());
				noInterrruptionBeforeMinDuration = lowerValue != "0" && lowerValue != "false";
			}

			std::vector<IGestureRecognizer*> recognizerRefs;
			rapidxml::xml_node<>* recRefNode;
			for(recRefNode = stateNode->first_node("Recognizer"); recRefNode; recRefNode = recRefNode->next_sibling("Recognizer"))
			{
				std::string name;
				rapidxml::xml_attribute<>* attr = recRefNode->first_attribute("name");
				if (attr)
				{
//
					if(findInStringPairVector(m_combinationRecognizers, rec->getName(), attr->value()) == -1)
						m_combinationRecognizers.push_back(pair <string, string>(rec->getName(), attr->value()));
//
					bool ignoreOnTrackingError = false;
					rapidxml::xml_attribute<>* attr1 = recRefNode->first_attribute("ignoreOnTrackingError");
					if (attr1)
					{
						std::string lowerValue = removeWhiteSpacesAndToLower(attr1->value());
						ignoreOnTrackingError = lowerValue != "0" && lowerValue != "false";
					}

					float minConf = -1.0f;
					rapidxml::xml_attribute<>* minConfA = recRefNode->first_attribute("minConfidence");
					if (minConfA)
						minConf = (float)atof(minConfA->value());

					int index = getUserDefinedRecognizerIndex(attr->value());
					if (index > -1)
					{
						// found it
						recognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
						recognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
						if (minConf >= 0)
							recognizerRefs.back()->m_minConfidence = minConf;
					}
					else
					{
						index = getHiddenUserDefinedRecognizerIndex(attr->value());
						if (index > -1)
						{
							// found it
							recognizerRefs.push_back(m_hiddenUserDefinedRecognizers[index].second->clone());
							recognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
							if (minConf >= 0)
								recognizerRefs.back()->m_minConfidence = minConf;
						}
						else
						{
							index = atoi(attr->value());
							if ((index > 0 || (index == 0 && attr->value()[0] == '0')) && (unsigned) index < m_userDefinedRecognizers.size())
							{
								// name in fact represents the index of a recognizer
								recognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
								recognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
								if (minConf >= 0)
									recognizerRefs.back()->m_minConfidence = minConf;
							}
							else // last option: name belongs to a predefined gesture
							{
								Postures::Posture p = getPostureID(attr->value());
								if (p < Postures::NUM_POSTURES)
								{
									// Found it
									recognizerRefs.push_back(createRecognizer(p));
									recognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
									if (minConf >= 0)
										recognizerRefs.back()->m_minConfidence = minConf;
									else if (globalMinConfidence >= 0)	
										// Only for predefined recognizers, global confidence is allowed to overwrite
										recognizerRefs.back()->m_minConfidence = globalMinConfidence;
								}
								else
								{
									// Finally not found
									Fubi_logErr("XML_Error - Unknown reference \"%s\" in \"%s\"!\n", attr->value(), rec->getName().c_str());
								}
							}
						}
					}

				}
			}

			std::vector<IGestureRecognizer*> notRecognizerRefs;
			rapidxml::xml_node<>* notRecRefNode;
			for(notRecRefNode = stateNode->first_node("NotRecognizer"); notRecRefNode; notRecRefNode = notRecRefNode->next_sibling("NotRecognizer"))
			{
				std::string name;
				rapidxml::xml_attribute<>* attr = notRecRefNode->first_attribute("name");
				if (attr)
				{
//
					if(findInStringPairVector(m_combinationRecognizers, rec->getName(), attr->value()) == -1)
						m_combinationRecognizers.push_back(pair <string, string>(rec->getName(), attr->value()));
//					
					// Default for not recognizers is true, as with a tracking error there is also no recognition
					bool ignoreOnTrackingError = true;
					rapidxml::xml_attribute<>* attr1 = notRecRefNode->first_attribute("ignoreOnTrackingError");
					if (attr1)
					{
						std::string lowerValue = removeWhiteSpacesAndToLower(attr1->value());
						ignoreOnTrackingError = lowerValue != "0" && lowerValue != "false";
					}

					float minConf = -1.0f;
					rapidxml::xml_attribute<>* minConfA = notRecRefNode->first_attribute("minConfidence");
					if (minConfA)
						minConf = (float)atof(minConfA->value());

					int index = getUserDefinedRecognizerIndex(attr->value());
					if (index > -1)
					{
						// found it
						notRecognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
						notRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
						if (minConf >= 0)
							notRecognizerRefs.back()->m_minConfidence = minConf;
					}
					else
					{
						index = getHiddenUserDefinedRecognizerIndex(attr->value());
						if (index > -1)
						{
							// found it
							notRecognizerRefs.push_back(m_hiddenUserDefinedRecognizers[index].second->clone());
							notRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
							if (minConf >= 0)
								notRecognizerRefs.back()->m_minConfidence = minConf;
						}
						else
						{
							index = atoi(attr->value());
							if ((index > 0 || (index == 0 && attr->value()[0] == '0')) && (unsigned) index < m_userDefinedRecognizers.size())
							{
								// name in fact represents the index of a recognizer
								notRecognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
								notRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
								if (minConf >= 0)
									notRecognizerRefs.back()->m_minConfidence = minConf;
							}
							else // last option: name belongs to a predefined gesture
							{
								Postures::Posture p = getPostureID(attr->value());
								if (p < Postures::NUM_POSTURES)
								{
									// Found it
									notRecognizerRefs.push_back(createRecognizer(p));
									notRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
									if (minConf >= 0)
										notRecognizerRefs.back()->m_minConfidence = minConf;
									else if (globalMinConfidence >= 0)	
										// Only for predefined recognizers, global confidence is allowed to overwrite
										notRecognizerRefs.back()->m_minConfidence = globalMinConfidence;
								}
								else
								{
									// Finally not found
									Fubi_logErr("XML_Error - Unknown reference \"%s\" in \"%s\"!\n", attr->value(), rec->getName().c_str());
								}
							}
						}
					}

				}
			}

			// Now check for alternative recognizers in the same way
			std::vector<IGestureRecognizer*> alternativeRecognizerRefs;
			std::vector<IGestureRecognizer*> alternativeNotRecognizerRefs;
			rapidxml::xml_node<>* alternativesNode = stateNode->first_node("AlternativeRecognizers");
			if (alternativesNode)
			{
				rapidxml::xml_node<>* alternativeRecRefNode;
				for(alternativeRecRefNode = alternativesNode->first_node("Recognizer"); alternativeRecRefNode; alternativeRecRefNode = alternativeRecRefNode->next_sibling("Recognizer"))
				{
					std::string name;
					rapidxml::xml_attribute<>* attr = alternativeRecRefNode->first_attribute("name");
					if (attr)
					{
//
						if(findInStringPairVector(m_combinationRecognizers, rec->getName(), attr->value()) == -1)
							m_combinationRecognizers.push_back(pair <string, string>(rec->getName(), attr->value()));
//
						bool ignoreOnTrackingError = false;
						rapidxml::xml_attribute<>* attr1 = alternativeRecRefNode->first_attribute("ignoreOnTrackingError");
						if (attr1)
						{
							std::string lowerValue = removeWhiteSpacesAndToLower(attr1->value());
							ignoreOnTrackingError = lowerValue != "0" && lowerValue != "false";
						}

						float minConf = -1.0f;
						rapidxml::xml_attribute<>* minConfA = alternativeRecRefNode->first_attribute("minConfidence");
						if (minConfA)
							minConf = (float)atof(minConfA->value());

						int index = getUserDefinedRecognizerIndex(attr->value());
						if (index > -1)
						{
							// found it
							alternativeRecognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
							alternativeRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
							if (minConf >= 0)
								alternativeRecognizerRefs.back()->m_minConfidence = minConf;
						}
						else
						{
							index = getHiddenUserDefinedRecognizerIndex(attr->value());
							if (index > -1)
							{
								// found it
								alternativeRecognizerRefs.push_back(m_hiddenUserDefinedRecognizers[index].second->clone());
								alternativeRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
								if (minConf >= 0)
									alternativeRecognizerRefs.back()->m_minConfidence = minConf;
							}
							else
							{
								index = atoi(attr->value());
								if ((index > 0 || (index == 0 && attr->value()[0] == '0')) && (unsigned) index < m_userDefinedRecognizers.size())
								{
									// name in fact represents the index of a recognizer
									alternativeRecognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
									alternativeRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
									if (minConf >= 0)
										alternativeRecognizerRefs.back()->m_minConfidence = minConf;
								}
								else // last option: name belongs to a predefined gesture
								{
									Postures::Posture p = getPostureID(attr->value());
									if (p < Postures::NUM_POSTURES)
									{
										// Found it
										alternativeRecognizerRefs.push_back(createRecognizer(p));
										alternativeRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
										if (minConf >= 0)
											alternativeRecognizerRefs.back()->m_minConfidence = minConf;
										else if (globalMinConfidence >= 0)	
											// Only for predefined recognizers, global confidence is allowed to overwrite
											alternativeRecognizerRefs.back()->m_minConfidence = globalMinConfidence;
									}
									else
									{
										// Finally not found
										Fubi_logErr("XML_Error - Unknown reference \"%s\" in \"%s\"!\n", attr->value(), rec->getName().c_str());
									}
								}
							}
						}

					}
				}

				rapidxml::xml_node<>* alternativeNotRecRefNode;
				for(alternativeNotRecRefNode = alternativesNode->first_node("NotRecognizer"); alternativeNotRecRefNode; alternativeNotRecRefNode = alternativeNotRecRefNode->next_sibling("NotRecognizer"))
				{
					std::string name;
					rapidxml::xml_attribute<>* attr = alternativeNotRecRefNode->first_attribute("name");
					if (attr)
					{
//
						if(findInStringPairVector(m_combinationRecognizers, rec->getName(), attr->value()) == -1)
							m_combinationRecognizers.push_back(pair <string, string>(rec->getName(), attr->value()));
//
						// Default for not recognizers is true, as with a tracking error there is also no recognition
						bool ignoreOnTrackingError = true;
						rapidxml::xml_attribute<>* attr1 = alternativeNotRecRefNode->first_attribute("ignoreOnTrackingError");
						if (attr1)
						{
							std::string lowerValue = removeWhiteSpacesAndToLower(attr1->value());
							ignoreOnTrackingError = lowerValue != "0" && lowerValue != "false";
						}

						float minConf = -1.0f;
						rapidxml::xml_attribute<>* minConfA = alternativeNotRecRefNode->first_attribute("minConfidence");
						if (minConfA)
							minConf = (float)atof(minConfA->value());

						int index = getUserDefinedRecognizerIndex(attr->value());
						if (index > -1)
						{
							// found it
							alternativeNotRecognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
							alternativeNotRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
							if (minConf >= 0)
								alternativeNotRecognizerRefs.back()->m_minConfidence = minConf;
						}
						else
						{
							index = getHiddenUserDefinedRecognizerIndex(attr->value());
							if (index > -1)
							{
								// found it
								alternativeNotRecognizerRefs.push_back(m_hiddenUserDefinedRecognizers[index].second->clone());
								alternativeNotRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
								if (minConf >= 0)
									alternativeNotRecognizerRefs.back()->m_minConfidence = minConf;
							}
							else
							{
								index = atoi(attr->value());
								if ((index > 0 || (index == 0 && attr->value()[0] == '0')) && (unsigned) index < m_userDefinedRecognizers.size())
								{
									// name in fact represents the index of a recognizer
									alternativeNotRecognizerRefs.push_back(m_userDefinedRecognizers[index].second->clone());
									alternativeNotRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
									if (minConf >= 0)
										alternativeNotRecognizerRefs.back()->m_minConfidence = minConf;
								}
								else // last option: name belongs to a predefined gesture
								{
									Postures::Posture p = getPostureID(attr->value());
									if (p < Postures::NUM_POSTURES)
									{
										// Found it
										alternativeNotRecognizerRefs.push_back(createRecognizer(p));
										alternativeNotRecognizerRefs.back()->m_ignoreOnTrackingError = ignoreOnTrackingError;
										if (minConf >= 0)
											alternativeNotRecognizerRefs.back()->m_minConfidence = minConf;
										else if (globalMinConfidence >= 0)	
											// Only for predefined recognizers, global confidence is allowed to overwrite
											alternativeNotRecognizerRefs.back()->m_minConfidence = globalMinConfidence;
									}
									else
									{
										// Finally not found
										Fubi_logErr("XML_Error - Unknown reference \"%s\" in \"%s\"!\n", attr->value(), rec->getName().c_str());
									}
								}
							}
						}

					}
				}
			}

			if (recognizerRefs.size() > 0 || notRecognizerRefs.size() > 0)
			{
				// Add state to the recognizer
				rec->addState(recognizerRefs, notRecognizerRefs, minDuration, maxDuration, timeForTransition, maxInterruption, noInterrruptionBeforeMinDuration, alternativeRecognizerRefs, alternativeNotRecognizerRefs);
			}
			else
			{
				// No recognizers in this state
				Fubi_logInfo("FubiCore: XML_Error - No references in state %d of rec \"%s\"!\n", stateNum, rec->getName().c_str());
			}
		}

		if (rec->getNumStates() > 0)
		{
			succes = true;
			// Add the recognizer to the templates
			m_userDefinedCombinationRecognizers.push_back(std::pair<string, CombinationRecognizer*>(name, rec));

			if (getAutoStartCombinationRecognition(Fubi::Combinations::NUM_COMBINATIONS))
			{
				// Enable it for all current users
				for (unsigned int user = 0; user < m_numUsers; user++)
				{
					m_users[user]->enableCombinationRecognition(rec, true);
				}
			}
		}
		else
			delete rec; // not passed to fubi control so we have to delete it ourselves
	}

	return succes;
}


void FubiCore::getDepthResolution(int& width, int& height)
{
	if (m_sensor)
	{
		width = m_sensor->getDepthOptions().m_width;
		height = m_sensor->getDepthOptions().m_height;
	}
	else
	{
		width = -1;
		height = -1;
	}
}
void FubiCore::getRgbResolution(int& width, int& height)
{
	if (m_sensor)
	{
		width = m_sensor->getRgbOptions().m_width;
		height = m_sensor->getRgbOptions().m_height;
	}
	else
	{
		width = -1;
		height = -1;
	}
}
void FubiCore::getIRResolution(int& width, int& height)
{
	if (m_sensor)
	{
		width = m_sensor->getIROptions().m_width;
		height = m_sensor->getIROptions().m_height;
	}
	else
	{
		width = -1;
		height = -1;
	}
}

unsigned int FubiCore::getClosestUserID()
{
	FubiUser* user = getClosestUser();

	if (user)
	{
		return user->m_id;
	}

	return 0;
}

FubiUser* FubiCore::getClosestUser()
{
	std::deque<FubiUser*> closestUsers = getClosestUsers();

	if (!closestUsers.empty())
	{
		// Take the closest tracked user for posture rec
		return closestUsers.front();
	}
	return 0x0;
}

std::deque<unsigned int> FubiCore::getClosestUserIDs(int maxNumUsers /*= -1*/)
{
	std::deque<unsigned int> closestUserIDs;

	// Get closest users
	std::deque<FubiUser*> closestUsers = getClosestUsers(maxNumUsers);

	// Copy their ids
	std::deque<FubiUser*>::iterator iter;
	std::deque<FubiUser*>::iterator end = closestUsers.end();
	for (iter = closestUsers.begin(); iter != end; iter++)
	{
		// Take the closest tracked user for posture rec
		closestUserIDs.push_back((*iter)->m_id);
	}

	return closestUserIDs;
}

std::deque<FubiUser*> FubiCore::getClosestUsers(int maxNumUsers /*= -1*/)
{
	// Copy array into vector
	std::deque<FubiUser*> closestUsers;

	if (maxNumUsers != 0)
	{
		closestUsers.insert(closestUsers.begin(), m_users, m_users + m_numUsers);

		// Sort vector with special operator according to their distance in the x-z plane
		std::sort(closestUsers.begin(), closestUsers.end(), FubiUser::closerToSensor);

		if (maxNumUsers > 0)
		{
			// Now remove users with largest distance to meet the max user criteria
			while(closestUsers.size() > (unsigned)maxNumUsers)
				closestUsers.pop_back();
			// And sort the rest additionally from left to right
			std::sort(closestUsers.begin(), closestUsers.end(), FubiUser::moreLeft);
		}
	}

	return closestUsers;
}


void FubiCore::clearUserDefinedRecognizers()
{
	for (unsigned int i = 0; i < Fubi::MaxUsers; i++)
	{
		m_users[i]->clearUserDefinedCombinationRecognizers();
	}

	vector<pair<string, CombinationRecognizer*> >::iterator iter1;
	vector<pair<string, CombinationRecognizer*> >::iterator end1 = m_userDefinedCombinationRecognizers.end();
	for (iter1 = m_userDefinedCombinationRecognizers.begin(); iter1 != end1; ++iter1)
	{
		delete iter1->second;
	}
	m_userDefinedCombinationRecognizers.clear();

	vector<pair<string, IGestureRecognizer*> >::iterator iter;
	vector<pair<string, IGestureRecognizer*> >::iterator end = m_userDefinedRecognizers.end();
	for (iter = m_userDefinedRecognizers.begin(); iter != end; ++iter)
	{
		delete iter->second;
	}
	m_userDefinedRecognizers.clear();

	vector<pair<string, IGestureRecognizer*> >::iterator iter2;
	vector<pair<string, IGestureRecognizer*> >::iterator end2 = m_hiddenUserDefinedRecognizers.end();
	for (iter2 = m_hiddenUserDefinedRecognizers.begin(); iter2 != end2; ++iter2)
	{
		delete iter2->second;
	}
	m_hiddenUserDefinedRecognizers.clear();
//
	m_jointsRecognizers.clear();
	m_jointsCombinations.clear();
	m_combinationRecognizers.clear();
	m_combinationSorted = false;
//
}

void FubiCore::updateTrackingData(unsigned int userId, float* skeleton, bool localOrientsValid /*= true*/,	double timeStamp /*= -1*/)
{
	if (skeleton != 0)
	{
		SkeletonJointPosition skelPositions[SkeletonJoint::NUM_JOINTS];
		SkeletonJointOrientation orientationMats[SkeletonJoint::NUM_JOINTS];
		SkeletonJointOrientation localOrientationMats[SkeletonJoint::NUM_JOINTS];
		for (int i = 0; i < SkeletonJoint::NUM_JOINTS; i++)
		{
			int startIndex = i*12;
			skelPositions[i].m_position.x = skeleton[startIndex];
			skelPositions[i].m_position.y = skeleton[startIndex+1];
			skelPositions[i].m_position.z = skeleton[startIndex+2];
			skelPositions[i].m_confidence = skeleton[startIndex+3];

			float rotX = skeleton[startIndex+4];
			float rotY = skeleton[startIndex+5];
			float rotZ = skeleton[startIndex+6];
			orientationMats[i].m_orientation = Matrix3f::RotMat(degToRad(rotX), degToRad(rotY), degToRad(rotZ));
			orientationMats[i].m_confidence = skeleton[startIndex+7];

			if (localOrientsValid)
			{
				rotX = skeleton[startIndex+8];
				rotY = skeleton[startIndex+9];
				rotZ = skeleton[startIndex+10];
				localOrientationMats[i].m_orientation = Matrix3f::RotMat(degToRad(rotX), degToRad(rotY), degToRad(rotZ));
				localOrientationMats[i].m_confidence = skeleton[startIndex+11];
			}
		}

		updateTrackingData(userId, skelPositions, timeStamp, orientationMats, localOrientsValid ? localOrientationMats : 0x0);
	}
}

void FubiCore::updateTrackingData(unsigned int userId, Fubi::SkeletonJointPosition* positions,
		double timeStamp /*= -1*/, Fubi::SkeletonJointOrientation* orientations /*= 0*/, Fubi::SkeletonJointOrientation* localOrientations /*= 0*/)
{
	// First check if this is a new user
	map<unsigned int, FubiUser*>::iterator iter = m_userIDToUsers.find(userId);
	int index = -1;
	FubiUser* user = 0x0;
	if (iter == m_userIDToUsers.end())
	{
		// new User, new entry
		index = m_numUsers;
		user = m_userIDToUsers[userId] = m_users[index];
		user->m_id = userId;
		m_numUsers++;

		// Init the user info
		user->m_inScene = true;
		user->m_isTracked = true;

		// Autostart posture combination detection
		for (unsigned int i = 0; i <Combinations::NUM_COMBINATIONS; ++i)
		{
			user->enableCombinationRecognition((Combinations::Combination)i,  getAutoStartCombinationRecognition((Combinations::Combination)i));
		}
		// Special treatment for user defined posture combinations
		if (getAutoStartCombinationRecognition(Fubi::Combinations::NUM_COMBINATIONS))
		{
			for (unsigned int j = 0; j < getNumUserDefinedCombinationRecognizers(); ++j)
			{
				user->enableCombinationRecognition(getUserDefinedCombinationRecognizer(j), true);
			}
		}
	}
	else
		user = iter->second;

	// Now set the new tracking info for the user and let him do the rest of the updates
	user->addNewTrackingData(positions, timeStamp, orientations, localOrientations);
}

Vec3f FubiCore::realWorldToProjective(const Vec3f& realWorldVec, int xRes /*= 640*/, int yRes /*= 480*/,
	double hFOV /*= 1.0144686707507438*/, double vFOV /*= 0.78980943449644714*/)
{
	Vec3f ret(Math::NO_INIT);

	if (m_sensor)
	{
		return m_sensor->realWorldToProjective(realWorldVec);
	}
	else
	{
		static const double realWorldXtoZ = tan(hFOV/2)*2;
		static const double realWorldYtoZ = tan(vFOV/2)*2;
		static const double coeffX = xRes / realWorldXtoZ;
		static const double coeffY = yRes / realWorldYtoZ;
		static const int nHalfXres = xRes / 2;
		static const int nHalfYres = yRes / 2;

		ret.x = (float)coeffX * realWorldVec.x / realWorldVec.z + nHalfXres;
		ret.y = nHalfYres - (float)coeffY * realWorldVec.y / realWorldVec.z;
		ret.z = realWorldVec.z;
	}

	return ret;
}

void FubiCore::resetTracking()
{
	if (m_sensor)
	{
		for (unsigned short i = 0; i < m_numUsers; ++i)
		{
			m_sensor->resetTracking(m_users[i]->m_id);
		}
	}
}

bool FubiCore::getImage(unsigned char* outputImage, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth,
		unsigned int renderOptions /*= (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions)*/,
		DepthImageModification::Modification depthModifications /*= DepthImageModification::UseHistogram*/,
        unsigned int userId /*= 0*/, Fubi::SkeletonJoint::Joint jointOfInterest /*= Fubi::SkeletonJoint::NUM_JOINTS*/)
{
    return FubiImageProcessing::getImage(m_sensor, outputImage, type, numChannels, depth, renderOptions, depthModifications, userId, jointOfInterest, m_current_gesture);
}

void FubiCore::setCurrentGesture(std::string gesture, unsigned int userId /*= 0*/)
{
    this->m_current_gesture[userId] = gesture;
}

bool FubiCore::saveImage(const char* fileName, int jpegQuality, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth,
	unsigned int renderOptions /*= (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions)*/,
	DepthImageModification::Modification depthModifications /*= DepthImageModification::UseHistogram*/,
	unsigned int userId /*= 0*/, Fubi::SkeletonJoint::Joint jointOfInterest /*= Fubi::SkeletonJoint::NUM_JOINTS*/)
{
	return FubiImageProcessing::saveImage(m_sensor, fileName, jpegQuality, type, numChannels, depth, renderOptions, depthModifications, userId, jointOfInterest);
}

void FubiCore::getColorForUserID(unsigned int id, float& r, float& g, float& b)
{
	FubiImageProcessing::getColorForUserID(id, r, g, b);
}

//// Added functions 2013/03/29
void FubiCore::printLoadedRecognizers()
{
	string name;
	vector <SkeletonJoint::Joint> sj;

	for(unsigned int i=0; i<m_jointsRecognizers.size(); i++)
	{
		name = m_jointsRecognizers[i].first;
		sj = m_jointsRecognizers[i].second;
		cout << name;
		for(unsigned int j=0; j<sj.size(); j++)
		{
			cout << " " << getJointName(sj[j]);
		}
		cout << endl;
	}
}

void FubiCore::printLoadedCombinations()
{
	for(unsigned int i=0; i<m_combinationRecognizers.size(); i++)
	{
		cout << m_combinationRecognizers[i].first << " " << m_combinationRecognizers[i].second << endl;
	}
}

void FubiCore::printLoadedJointsCombinations()
{
	vector<Fubi::SkeletonJoint::Joint> joints;
	for(unsigned int i=0; i<m_jointsCombinations.size(); i++)
	{
		cout << m_jointsCombinations[i].first;
		joints = m_jointsCombinations[i].second;
		cout << " nb joints: " << joints.size();
		for(unsigned int j=0; j<joints.size(); j++)
			cout << " " << getJointName(joints[j]);
		cout << endl;
	}
}


void FubiCore::combinationRecToJoints()
{
	vector<string> comboNames;
	string comboName;
	for(unsigned int i=0; i<m_combinationRecognizers.size(); i++)
	{
		comboName = m_combinationRecognizers[i].first;
		if(findInStringVector(comboNames, comboName) == -1)
			comboNames.push_back(comboName);
	}
	vector<Fubi::SkeletonJoint::Joint> joints;
	for(unsigned int i=0; i<comboNames.size(); i++)
        m_jointsCombinations.push_back(pair<string, vector<Fubi::SkeletonJoint::Joint> >(comboNames[i], joints));

	string recoName;
	vector<Fubi::SkeletonJoint::Joint> comboJoints;
	vector<Fubi::SkeletonJoint::Joint> recJoints;
	for(unsigned int i=0; i<m_jointsCombinations.size(); i++)
	{
		comboJoints.clear();
		for(unsigned int j=0; j<m_combinationRecognizers.size(); j++)
		{
			comboName = m_combinationRecognizers[j].first;
			recoName = m_combinationRecognizers[j].second;
			if(m_jointsCombinations[i].first == comboName)
			{
				for(unsigned int k=0; k<m_jointsRecognizers.size(); k++)
				{
					if(m_jointsRecognizers[k].first == recoName)
					{
						recJoints = m_jointsRecognizers[k].second;
						for(unsigned int l=0; l<recJoints.size(); l++)
						{
							if(findInJointVector(comboJoints, recJoints[l]) == -1)
								comboJoints.push_back(recJoints[l]);
						}
					}
				}
			}
		}
		m_jointsCombinations[i].second = comboJoints;
	}
	m_combinationSorted = true;
	//printLoadedJointsCombinations();
}


int FubiCore::findInStringVector(vector<string> vec, string s)
{
	for(unsigned int i=0; i<vec.size(); i++)
	{
		if(vec[i] == s)
			return i;
	}
	return -1;
}

int FubiCore::findInJointVector(vector<Fubi::SkeletonJoint::Joint> vec, Fubi::SkeletonJoint::Joint j)
{
	for(unsigned int i=0; i<vec.size(); i++)
	{
		if(vec[i] == j)
			return i;
	}
	return -1;
}

int FubiCore::findInStringPairVector(vector<pair<string, string> > vec, string s1, string s2)
{
	for(unsigned int i=0; i<vec.size(); i++)
	{
		if(vec[i].first == s1 && vec[i].second == s2)
			return i;
	}
	return -1;
}
