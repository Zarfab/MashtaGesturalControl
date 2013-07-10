// ****************************************************************************************
//
// Fubi User
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************
#pragma once

#include "FubiPredefinedGestures.h"
#include "FubiUtils.h"

#include <map>
#include <deque>
#include <queue>

class CombinationRecognizer;

// The FubiUser class hold all relevant informations for each tracked user
class FubiUser
{
public:
	FubiUser();
	~FubiUser();

	// operator used for comparing the user according to their distance to the sensor
	// in the x-z plane --> get the closest users
	static bool closerToSensor(const FubiUser* u1, const FubiUser* u2);

	// operator used for comparing which user is more left of the sensor
	static bool moreLeft(const FubiUser* u1, const FubiUser* u2);

	// Enables/disables a posture combination recognizer of this user
	void enableCombinationRecognition(Fubi::Combinations::Combination postureID, bool enable);
	void enableCombinationRecognition(const CombinationRecognizer* recognizerTemplate, bool enable);

	// Enable/disables the tracking of the shown number of fingers for each hand
	void enableFingerTracking(bool leftHand, bool rightHand, bool useConvexityDefectMethod = false);

	// Gets the finger count optionally calculated by the median of the last 10 calculations
	int getFingerCount(bool leftHand = false, bool getMedianOfLastFrames = true, bool useOldConvexityDefectMethod = false);

	// Stops and removes all user defined 
	void clearUserDefinedCombinationRecognizers();

	// Update the tracking info from the given sensor
	void updateTrackingData(class FubiISensor* sensor);

	// Reset the user to an initial state
	void reset();

	// Manually update the tracking info
	void addNewTrackingData(Fubi::SkeletonJointPosition* positions,
		double timeStamp = -1, Fubi::SkeletonJointOrientation* orientations = 0, Fubi::SkeletonJointOrientation* localOrientations = 0);

	const Fubi::FingerCountImageData* getFingerCountImageData(bool left = false)
	{
		return left ? &m_leftFingerCountImage : &m_rightFingerCountImage;
	}

	// Whether the user is currently seen in the depth image
	bool m_inScene;

	// OpenNI id of this user
	unsigned int m_id;

	// Whether the user is currently tracked
	bool m_isTracked;
	
	// Current skeleton joints (position/orientation) per user with a timestamp
	struct TrackingData
	{
		Fubi::SkeletonJointPosition jointPositions[Fubi::SkeletonJoint::NUM_JOINTS];
		Fubi::SkeletonJointPosition localJointPositions[Fubi::SkeletonJoint::NUM_JOINTS];
		Fubi::SkeletonJointOrientation jointOrientations[Fubi::SkeletonJoint::NUM_JOINTS];
		Fubi::SkeletonJointOrientation localJointOrientations[Fubi::SkeletonJoint::NUM_JOINTS];
		double timeStamp;
	};
	TrackingData m_currentTrackingData, m_lastTrackingData;

	// The user's body measurements
	Fubi::BodyMeasurementDistance m_bodyMeasurements[Fubi::BodyMeasurement::NUM_MEASUREMENTS];
	double m_lastBodyMeasurementUpdate;

	// One posture combination recognizer per posture combination
	CombinationRecognizer* m_combinationRecognizers[Fubi::Combinations::NUM_COMBINATIONS];
	// And all user defined ones that are currently enabled
	std::map<std::string, CombinationRecognizer*> m_userDefinedCombinationRecognizers;

	// Time between the finger count detection of one hand
	double m_fingerTrackIntervall;

	// When the last detection of the finger count of each hand happened, -1 if disabled
	double m_lastRightFingerDetection, m_lastLeftFingerDetection;
	bool m_useConvexityDefectMethod;
	unsigned int m_maxFingerCountForMedian;

private:
	// Adds a finger count detection to the deque for later median calculation
	void addFingerCount(int count, bool leftHand = false);


	void calculateGlobalOrientations();

	void calculateLocalTransformations();

	void updateCombinationRecognizers();

	void updateFingerCount();

	int calculateMedianFingerCount(const std::deque<int>& fingerCount);

	void updateBodyMeasurements();

	std::deque<int> m_rightFingerCount, m_leftFingerCount;		

	Fubi::FingerCountImageData m_leftFingerCountImage, m_rightFingerCountImage;
};