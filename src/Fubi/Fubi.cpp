// ****************************************************************************************
//
// Fubi API
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html 
// 
// 
// ****************************************************************************************

#include "Fubi.h"
#include "FubiCore.h"
#include "FubiConfig.h"

#include <deque>

namespace Fubi
{
	FUBI_API bool init(const char* xmlPath /*=0x0*/, Fubi::SkeletonTrackingProfile::Profile profile /*= Fubi::SkeletonTrackingProfile::ALL*/,
		bool mirrorStream /*= true*/, float smoothing /*= 0*/)
	{
		return FubiCore::init(xmlPath, profile, mirrorStream, smoothing);
	}

	FUBI_API bool init(const SensorOptions& options)
	{
		return FubiCore::init(options);
	}


	FUBI_API bool init(int depthWidth, int depthHeight, int depthFPS /*= 30*/,
		int rgbWidth /*= 640*/, int rgbHeight /*= 480*/, int rgbFPS /*= 30*/,
		int irWidth /*= -1*/, int irHeight /*= -1*/, int irFPS /*= -1*/,
		Fubi::SensorType::Type sensorType /*= Fubi::SensorType::OPENNI2*/,
		Fubi::SkeletonTrackingProfile::Profile profile /*= Fubi::SkeletonTrackingProfile::ALL*/,
		bool mirrorStream /*= true*/, float smoothing /*= 0*/)
	{
		return init(SensorOptions(StreamOptions(depthWidth, depthHeight, depthFPS), StreamOptions(rgbWidth, rgbHeight, rgbFPS), StreamOptions(irWidth, irHeight, irFPS), sensorType,
			profile, mirrorStream, smoothing));
	}

	FUBI_API bool switchSensor(const SensorOptions& options)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->initSensorWithOptions(options);
		return false;
	}

	FUBI_API bool switchSensor(Fubi::SensorType::Type sensorType, int depthWidth, int depthHeight, int depthFPS /*= 30*/,
		int rgbWidth /*= 640*/, int rgbHeight /*= 480*/, int rgbFPS /*= 30*/,
		int irWidth /*= -1*/, int irHeight /*= -1*/, int irFPS /*= -1*/,
		Fubi::SkeletonTrackingProfile::Profile profile /*= Fubi::SkeletonTrackingProfile::ALL*/,
		bool mirrorStream /*= true*/, float smoothing /*= 0*/)
	{
		return switchSensor(SensorOptions(StreamOptions(depthWidth, depthHeight, depthFPS), StreamOptions(rgbWidth, rgbHeight, rgbFPS), StreamOptions(irWidth, irHeight, irFPS), sensorType,
			profile, mirrorStream, smoothing));
	}

	FUBI_API Fubi::SensorType::Type getCurrentSensorType()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core && core->getSensor())
			return core->getSensor()->getType();
		return Fubi::SensorType::NONE;
	}

	FUBI_API int getAvailableSensorTypes()
	{
		int ret = 0;
#ifdef USE_OPENNI2
		ret |= SensorType::OPENNI2;
#endif
#ifdef USE_OPENNI1
		ret |= SensorType::OPENNI1;
#endif
#ifdef USE_KINECT_SDK
		ret |= SensorType::KINECTSDK;
#endif
		return ret;
	}

	FUBI_API void release()
	{
		FubiCore::release();
	}

	FUBI_API void updateSensor()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			core->updateSensor();
	}

	FUBI_API bool getImage(unsigned char* outputImage, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth,
		unsigned int renderOptions /*= (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions)*/,
		DepthImageModification::Modification depthModifications /*= DepthImageModification::UseHistogram*/,
		unsigned int userId/* = 0*/, Fubi::SkeletonJoint::Joint jointOfInterest /*= Fubi::SkeletonJoint::NUM_JOINTS*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getImage(outputImage, type, numChannels, depth, renderOptions, depthModifications, userId, jointOfInterest);
		return false;
	}

	FUBI_API bool saveImage(const char* fileName, int jpegQuality, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth,
		unsigned int renderOptions /*= (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions)*/,
		DepthImageModification::Modification depthModifications /*= DepthImageModification::UseHistogram*/,
		unsigned int userId /*= 0*/, Fubi::SkeletonJoint::Joint jointOfInterest /*= Fubi::SkeletonJoint::NUM_JOINTS*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->saveImage(fileName, jpegQuality, type, numChannels, depth, renderOptions, depthModifications, userId, jointOfInterest);
		return false;
	}

	FUBI_API bool isInitialized()
	{
		return (FubiCore::getInstance() != 0x0);
	}

	FUBI_API Fubi::RecognitionResult::Result recognizeGestureOn(Postures::Posture postureID, unsigned int userID)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->recognizeGestureOn(postureID, userID);
		return Fubi::RecognitionResult::NOT_RECOGNIZED;
	}

	FUBI_API Fubi::RecognitionResult::Result recognizeGestureOn(unsigned int recognizerIndex, unsigned int userID)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->recognizeGestureOn(recognizerIndex, userID);
		return Fubi::RecognitionResult::NOT_RECOGNIZED;
	}

	FUBI_API Fubi::RecognitionResult::Result recognizeGestureOn(const char* recognizerName, unsigned int userID)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core && recognizerName)
			return core->recognizeGestureOn(recognizerName, userID);
		return Fubi::RecognitionResult::NOT_RECOGNIZED;
	}

	FUBI_API Fubi::RecognitionResult::Result getCombinationRecognitionProgressOn(Combinations::Combination combinationID, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates /*= 0x0*/, bool restart /*= true*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getCombinationRecognitionProgressOn(combinationID, userID, userStates, restart);
		return Fubi::RecognitionResult::NOT_RECOGNIZED;
	}

	FUBI_API Fubi::RecognitionResult::Result getCombinationRecognitionProgressOn(const char* recognizerName, unsigned int userID, std::vector<FubiUser::TrackingData>* userStates /*= 0x0*/, bool restart /*= true*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core && recognizerName)
			return core->getCombinationRecognitionProgressOn(recognizerName, userID, userStates, restart);
		return Fubi::RecognitionResult::NOT_RECOGNIZED;
	}

	FUBI_API void enableCombinationRecognition(Combinations::Combination combinationID, unsigned int userID, bool enable)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			core->enableCombinationRecognition(combinationID, userID, enable);
	}

	FUBI_API void enableCombinationRecognition(const char* combinationName, unsigned int userID, bool enable)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core && combinationName)
			core->enableCombinationRecognition(combinationName, userID, enable);
	}

	FUBI_API void setAutoStartCombinationRecognition(bool enable, Combinations::Combination combinationID /*= Combinations::NUM_COMBINATIONS*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			core->setAutoStartCombinationRecognition(enable, combinationID);
	}

	FUBI_API bool getAutoStartCombinationRecognition(Combinations::Combination combinationID /*= Combinations::NUM_COMBINATIONS*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getAutoStartCombinationRecognition(combinationID);
		return false;
	}

	FUBI_API void getColorForUserID(unsigned int id, float& r, float& g, float& b)
	{
		FubiCore::getColorForUserID(id, r, g, b);
	}


	FUBI_API unsigned int getUserID(unsigned int index)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getUserID(index);
		return 0;
	}

	FUBI_API unsigned int addJointRelationRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint,
		float minX, float minY, float minZ /*= -Fubi::Math::MaxFloat,-Fubi::Math::MaxFloat, -Fubi::Math::MaxFloat*/, 
		float maxX, float maxY, float maxZ /*= Fubi::Math::MaxFloat, Fubi::Math::MaxFloat, Fubi::Math::MaxFloat*/, 
		float minDistance /*= 0*/, 
		float maxDistance /*= Fubi::Math::MaxFloat*/,
		bool uselocalPositions /*= false*/,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float minConfidence /*= -1.0f*/,
		Fubi::BodyMeasurement::Measurement measuringUnit /*= Fubi::BodyMeasurement::NUM_MEASUREMENTS*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->addJointRelationRecognizer(joint, relJoint, Vec3f(minX, minY, minZ), Vec3f(maxX, maxY, maxZ), minDistance, maxDistance, uselocalPositions, atIndex, name, minConfidence, measuringUnit);
		return -1;
	}

	FUBI_API unsigned int addJointOrientationRecognizer(Fubi::SkeletonJoint::Joint joint,
		float minX /*= -180.0f*/, float minY /*= -180.0f*/, float minZ /*= -180.0f*/,
		float maxX /*= 180.0f*/, float maxY /*= 180.0f*/, float maxZ /*= 180.0f*/,
		bool useLocalOrientations /*= true*/,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float minConfidence /*=-1*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->addJointOrientationRecognizer(joint, Vec3f(minX, minY, minZ), Vec3f(maxX, maxY, maxZ), useLocalOrientations, atIndex, name, minConfidence);
		return -1;
	}

	unsigned int addFingerCountRecognizer(Fubi::SkeletonJoint::Joint handJoint,
		unsigned int minFingers, unsigned int maxFingers,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float minConfidence /*=-1*/,
		bool useMedianCalculation /*= false*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->addFingerCountRecognizer(handJoint, minFingers, maxFingers, atIndex, name, minConfidence, useMedianCalculation);
		return -1;
	}

	FUBI_API unsigned int addLinearMovementRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint, 
		float dirX, float dirY, float dirZ, float minVel, float maxVel /*= Fubi::Math::MaxFloat*/,
		bool uselocalPositions /*= false*/,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float maxAngleDifference /*= 45.0f*/, 
		bool useOnlyCorrectDirectionComponent /*= true*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->addLinearMovementRecognizer(joint, relJoint, Vec3f(dirX, dirY, dirZ), minVel, maxVel, uselocalPositions, atIndex, name, -1.0f, maxAngleDifference, useOnlyCorrectDirectionComponent);
		return -1;
	}
	FUBI_API unsigned int addLinearMovementRecognizer(SkeletonJoint::Joint joint, 
		float dirX, float dirY, float dirZ, float minVel, float maxVel /*= Fubi::Math::MaxFloat*/,
		bool uselocalPositions /*= false*/,
		int atIndex /*= -1*/,
		const char* name /*= 0*/,
		float maxAngleDifference /*= 45.0f*/, 
		bool useOnlyCorrectDirectionComponent /*= true*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->addLinearMovementRecognizer(joint, Vec3f(dirX, dirY, dirZ), minVel, maxVel, uselocalPositions, atIndex, name , -1.0f, maxAngleDifference, useOnlyCorrectDirectionComponent);
		return -1;
	}

	FUBI_API bool addCombinationRecognizer(const char* xmlDefinition)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core && xmlDefinition)
			return core->addCombinationRecognizer(xmlDefinition);
		return false;
	}

	FUBI_API bool loadRecognizersFromXML(const char* fileName)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core && fileName)
			return core->loadRecognizersFromXML(fileName);
		return false;
	}

	FUBI_API unsigned int getNumUserDefinedRecognizers()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getNumUserDefinedRecognizers();
		return 0;
	}

	FUBI_API const char* getUserDefinedRecognizerName(unsigned int recognizerIndex)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getUserDefinedRecognizerName(recognizerIndex).c_str();
		return "";
	}

	FUBI_API int getUserDefinedRecognizerIndex(const char* recognizerName)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getUserDefinedRecognizerIndex(recognizerName);
		return -1;
	}

	FUBI_API unsigned int getNumUserDefinedCombinationRecognizers()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getNumUserDefinedCombinationRecognizers();
		return 0;
	}

	FUBI_API const char* getUserDefinedCombinationRecognizerName(unsigned int recognizerIndex)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getUserDefinedCombinationRecognizerName(recognizerIndex).c_str();
		return "";
	}

	FUBI_API int getUserDefinedCombinationRecognizerIndex(const char* recognizerName)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getUserDefinedCombinationRecognizerIndex(recognizerName);
		return -1;
	}

	FUBI_API unsigned short getCurrentUsers(FubiUser*** pUserContainer /*= 0*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getCurrentUsers(pUserContainer);

		return 0;
	}

	FUBI_API FubiUser* getUser(unsigned int id)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
			return core->getUser(id);

		return 0;
	}

	FUBI_API void getDepthResolution(int& width, int& height)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->getDepthResolution(width, height);
		}
	}

	
	FUBI_API void getRgbResolution(int& width, int& height)
		{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->getRgbResolution(width, height);
		}
	}

	FUBI_API void getIRResolution(int& width, int& height)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->getIRResolution(width, height);
		}
	}

	FUBI_API int getFingerCount(unsigned int userID, bool leftHand /*= false*/, bool getMedianOfLastFrames /*= true*/, bool useOldConvexityDefectMethod /*= false*/)
	{
		int numFingers = -1;

		FubiUser* user = getUser(userID);
		if (user)
			numFingers = user->getFingerCount(leftHand, getMedianOfLastFrames, useOldConvexityDefectMethod);

		return numFingers;
	}

	FUBI_API void enableFingerTracking(unsigned int userID, bool leftHand, bool rightHand, bool useConvexityDefectMethod /*= false*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			FubiUser* user = core->getUser(userID);
			if (user)
			{
				user->enableFingerTracking(leftHand, rightHand, useConvexityDefectMethod);
			}
		}
	}

	FUBI_API bool isUserInScene(unsigned int userID)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			FubiUser* user = core->getUser(userID);
			if (user)
			{
				return user->m_inScene;
			}
		}
		return false;
	}

	FUBI_API bool isUserTracked(unsigned int userID)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			FubiUser* user = core->getUser(userID);
			if (user)
			{
				return user->m_isTracked;
			}
		}
		return false;
	}
	

	FUBI_API FubiUser::TrackingData* getCurrentTrackingData(unsigned int userId)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			FubiUser* user = core->getUser(userId);
			if (user)
			{
				return &user->m_currentTrackingData;
			}
		}
		return 0;
	}

	FUBI_API FubiUser::TrackingData* getLastTrackingData(unsigned int userId)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			FubiUser* user = core->getUser(userId);
			if (user)
			{
				return &user->m_lastTrackingData;
			}
		}
		return 0;
	}
		
		
	FUBI_API void getSkeletonJointPosition(FubiUser::TrackingData* trackingData, SkeletonJoint::Joint joint, float& x, float& y, float& z, float& confidence, double& timeStamp, bool localPosition /*= false*/)
	{
		if (trackingData)
		{
			SkeletonJointPosition jointPos;
			if (localPosition)
				jointPos = trackingData->localJointPositions[joint];
			else
				jointPos = trackingData->jointPositions[joint];
			x = jointPos.m_position.x;
			y = jointPos.m_position.y;
			z = jointPos.m_position.z;
			confidence = jointPos.m_confidence;
			timeStamp = trackingData->timeStamp;
		}
	}

	FUBI_API void getSkeletonJointOrientation(FubiUser::TrackingData* trackingData, SkeletonJoint::Joint joint, float* mat, float confidence, double& timeStamp, bool localOrientation /*= true*/)
	{
		if (trackingData && mat)
		{
			SkeletonJointOrientation jointOrient;
			if (localOrientation)
				jointOrient = trackingData->localJointOrientations[joint];
			else
				jointOrient = trackingData->jointOrientations[joint];
			for (int i = 0; i < 9; ++i)
			{
				mat[i] = jointOrient.m_orientation.x[i];
			}
			confidence = jointOrient.m_confidence;
			timeStamp = trackingData->timeStamp;
		}
	}

	FUBI_API std::vector<FubiUser::TrackingData>* createTrackingDataVector()
	{
		return new std::vector<FubiUser::TrackingData>();
	}
	
	FUBI_API void releaseTrackingDataVector(std::vector<FubiUser::TrackingData>* vec)
	{
		delete vec;
	}

	FUBI_API unsigned int getTrackingDataVectorSize(std::vector<FubiUser::TrackingData>* vec)
	{
		if (vec)
			return vec->size();
		return 0;
	}

	FUBI_API FubiUser::TrackingData* getTrackingData(std::vector<FubiUser::TrackingData>* vec, unsigned int index)
	{
		if (vec)
			return &((*vec)[index]);
		return 0;
	}

	FUBI_API unsigned int getClosestUserID()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			return core->getClosestUserID();
		}
		return 0;
	}

	FUBI_API std::deque<unsigned int> getClosestUserIDs(int maxNumUsers /*= -1*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			return core->getClosestUserIDs(maxNumUsers);
		}
		static std::deque<unsigned int> s_emptyDeque;
		return s_emptyDeque;
	}

	FUBI_API FubiUser* getClosestUser()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			return core->getClosestUser();
		}
		return 0x0;
	}

	FUBI_API std::deque<FubiUser*> getClosestUsers(int maxNumUsers /*= -1*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			return core->getClosestUsers(maxNumUsers);
		}
		static std::deque<FubiUser*> s_emptyDeque;
		return s_emptyDeque;
	}

	FUBI_API void clearUserDefinedRecognizers()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->clearUserDefinedRecognizers();
		}
	}

	FUBI_API void updateTrackingData(unsigned int userId, Fubi::SkeletonJointPosition* positions,
		double timeStamp /*= -1*/, Fubi::SkeletonJointOrientation* orientations /*= 0*/, Fubi::SkeletonJointOrientation* localOrientations /*= 0*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->updateTrackingData(userId, positions, timeStamp, orientations, localOrientations);
		}
	}

	FUBI_API void updateTrackingData(unsigned int userId, float* skeleton, bool localOrientsValid /*= true*/,
		double timeStamp /*= -1*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->updateTrackingData(userId, skeleton, localOrientsValid, timeStamp);
		}
	}

	FUBI_API Fubi::Vec3f realWorldToProjective(const Fubi::Vec3f& realWorldVec, int xRes /*= 640*/, int yRes /*= 480*/,
		double hFOV /*= 1.0144686707507438*/, double vFOV /*= 0.78980943449644714*/)
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			return core->realWorldToProjective(realWorldVec, xRes, yRes, hFOV, vFOV);
		}
		return Fubi::Vec3f();
	}

	FUBI_API void realWorldToProjective(float realWorldX, float realWorldY, float realWorldZ, float& screenX, float& screenY, float& screenZ,
		int xRes /*= 640*/, int yRes /*= 480*/, double hFOV /*= 1.0144686707507438*/, double vFOV /*= 0.78980943449644714*/)
	{
		Fubi::Vec3f result = realWorldToProjective(Fubi::Vec3f(realWorldX, realWorldY, realWorldZ), xRes, xRes, hFOV, vFOV);
		screenX = result.x;
		screenY = result.y;
		screenZ = result.z;
	}
//
    FUBI_API std::vector<std::pair<std::string, std::vector<Fubi::SkeletonJoint::Joint> > > getCombinations()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			if(!core->m_combinationSorted)
				core->combinationRecToJoints();
			return core->m_jointsCombinations;
		}
	}
	
	FUBI_API std::vector<Fubi::SkeletonJoint::Joint> getComboJoints(std::string comboName)
	{
        std::vector<std::pair<std::string, std::vector<Fubi::SkeletonJoint::Joint> > > combinations = getCombinations();
		for(unsigned int i=0; i<combinations.size(); i++)
		{
			if(combinations[i].first == comboName)
				return combinations[i].second;
		}
		std::vector<Fubi::SkeletonJoint::Joint> j;
		return j;
	}
//
	FUBI_API void resetTracking()
	{
		FubiCore* core = FubiCore::getInstance();
		if (core)
		{
			core->resetTracking();
		}
	}

	FUBI_API double getCurrentTime()
	{
		return currentTime();
	}
}
