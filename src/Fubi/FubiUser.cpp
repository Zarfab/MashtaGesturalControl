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

#include "FubiUser.h"

#include "FubiISensor.h"
#include "FubiUtils.h"
#include "FubiCore.h"
#include "FubiImageProcessing.h"
#include "FubiRecognizerFactory.h"
#include "GestureRecognizer/CombinationRecognizer.h"

using namespace Fubi;

FubiUser::FubiUser() : m_inScene(false), m_id(0), m_isTracked(false),
	m_lastRightFingerDetection(-1), m_lastLeftFingerDetection(-1), m_fingerTrackIntervall(0.1),
	m_maxFingerCountForMedian(10), m_useConvexityDefectMethod(false),
	m_lastBodyMeasurementUpdate(0)
{
	//  Init tracking data timestamps
	m_currentTrackingData.timeStamp = 0;
	m_lastTrackingData.timeStamp = 0;

	// Init the posture combination recognizers
	for (unsigned int i = 0; i < Combinations::NUM_COMBINATIONS; ++i)
	{
		m_combinationRecognizers[i] = Fubi::createRecognizer(this, (Combinations::Combination) i);
	}
}


FubiUser::~FubiUser()
{
	for (unsigned int i = 0; i < Combinations::NUM_COMBINATIONS; ++i)
	{
		delete m_combinationRecognizers[i];
		m_combinationRecognizers[i] = 0x0;
	}

	clearUserDefinedCombinationRecognizers();

	// Release any left over image data
	FubiImageProcessing::releaseImage(m_leftFingerCountImage.image);
	FubiImageProcessing::releaseImage(m_rightFingerCountImage.image);
}

bool FubiUser::closerToSensor(const FubiUser* u1, const FubiUser* u2)
{
	const SkeletonJointPosition& pos1 = u1->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];
	const SkeletonJointPosition& pos2 = u2->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];

	if (u1->m_isTracked && pos1.m_confidence > 0.1f)
	{
		if (u2->m_isTracked  && pos2.m_confidence > 0.1f)
		{
			// Compare their distance (int the x,z-plane) to the sensor
			float dist1 = sqrtf(pos1.m_position.z*pos1.m_position.z + pos1.m_position.x*pos1.m_position.x);
			float dist2 = sqrtf(pos2.m_position.z*pos2.m_position.z + pos2.m_position.x*pos2.m_position.x);
			return dist1 < dist2;
		}
		else
		{
			// u1 is "closer" to the sensor (only valid user)
			return true;
		}
	}
	else if (u2->m_isTracked  && pos2.m_confidence > 0.1f)
	{
		return false; // u2 is "closer" to the sensor (only valid user)
	}

	// No valid user -> comparison has no meaning
	// but we compare the id to retain a strict weak ordering
	return u1->m_id < u2->m_id;
}

bool FubiUser::moreLeft(const FubiUser* u1, const FubiUser* u2)
{
	const SkeletonJointPosition& pos1 = u1->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];
	const SkeletonJointPosition& pos2 = u2->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];

	if (u1->m_isTracked && pos1.m_confidence > 0.1f)
	{
		if (u2->m_isTracked  && pos2.m_confidence > 0.1f)
		{
			// Compare their x value
			return pos1.m_position.x < pos2.m_position.x;
		}
		else
		{
			// u1 is "more left" to the sensor (only valid user)
			return true;
		}
	}
	else if (u2->m_isTracked  && pos2.m_confidence > 0.1f)
	{
		return false; // u2 is "more left" to the sensor (only valid user)
	}

	// No valid user -> comparison has no meaning
	// but we compare the id to retain a strict weak ordering
	return u1->m_id < u2->m_id;
}

void FubiUser::clearUserDefinedCombinationRecognizers()
{
	std::map<std::string, CombinationRecognizer*>::iterator iter;
	std::map<std::string, CombinationRecognizer*>::iterator end = m_userDefinedCombinationRecognizers.end();
	for (iter = m_userDefinedCombinationRecognizers.begin(); iter != end; ++iter)
	{
		delete iter->second;
	}
	m_userDefinedCombinationRecognizers.clear();
}



void FubiUser::enableCombinationRecognition(Fubi::Combinations::Combination postureID, bool enable)
{
	if (postureID < Fubi::Combinations::NUM_COMBINATIONS && m_combinationRecognizers[postureID])
	{
		if (enable)
			m_combinationRecognizers[postureID]->start();
		else
			m_combinationRecognizers[postureID]->stop();
	}
}

void FubiUser::enableCombinationRecognition(const CombinationRecognizer* recognizerTemplate, bool enable)
{
	if (recognizerTemplate)
	{
		std::map<std::string, CombinationRecognizer*>::iterator rec = m_userDefinedCombinationRecognizers.find(recognizerTemplate->getName());
		if (rec != m_userDefinedCombinationRecognizers.end())
		{
			if (enable)
				rec->second->start();
			else
			{
				rec->second->stop();
			}
		}
		else if (enable)
		{
			CombinationRecognizer* clonedRec = recognizerTemplate->clone();
			clonedRec->setUser(this);
			m_userDefinedCombinationRecognizers[recognizerTemplate->getName()] = clonedRec;
			clonedRec->start();
		}
	}
}

void FubiUser::enableFingerTracking(bool leftHand, bool rightHand, bool useConvexityDefectMethod /*= false*/)
{
	bool enabledAnything = false;
	if (leftHand)
	{
		if (m_lastLeftFingerDetection == -1)
		{
			m_leftFingerCount.clear();
			m_lastLeftFingerDetection = 0;
			enabledAnything = true;
		}
	}
	else
		m_lastLeftFingerDetection = -1;

	if (rightHand)
	{
		if(m_lastRightFingerDetection == -1)
		{
			m_rightFingerCount.clear();
			m_lastRightFingerDetection = 0;
			enabledAnything = true;
		}
	}
	else
		m_lastRightFingerDetection = -1;

	if (enabledAnything)
		// Immediatley update the finger count
		updateFingerCount();
}

void FubiUser::addFingerCount(int count, bool leftHand /*= false*/)
{
	if (leftHand)
	{
		if (m_lastLeftFingerDetection > -1)
		{
			if (count > -1)
			{
				m_leftFingerCount.push_back(count);
				if (m_leftFingerCount.size() > m_maxFingerCountForMedian)
					m_leftFingerCount.pop_front();
			}
			m_lastLeftFingerDetection = Fubi::currentTime();
		}
	}
	else
	{
		if (m_lastRightFingerDetection > -1)
		{
			if (count > -1)
			{
				m_rightFingerCount.push_back(count);
				if (m_rightFingerCount.size() > m_maxFingerCountForMedian)
					m_rightFingerCount.pop_front();
			}
			m_lastRightFingerDetection = Fubi::currentTime();
		}
	}
}

int FubiUser::calculateMedianFingerCount(const std::deque<int>& fingerCount)
{
	int median = -1;
	std::priority_queue<int> sortedQueue;
	
	// Sort values in a queue
	std::deque<int>::const_iterator it;
	std::deque<int>::const_iterator end = fingerCount.end();
	for (it = fingerCount.begin(); it != end; ++it)
	{
		sortedQueue.push(*it);
	}

	// Throw away first half of the sorted queue
	int half = sortedQueue.size() / 2;
	for (int i = 0; i < half; ++i)
	{
		sortedQueue.pop();		
	}

	// Median is now on top
	if (!sortedQueue.empty())
	{
		median = sortedQueue.top();
	}

	return median;
}

int FubiUser::getFingerCount(bool leftHand /*= false*/, bool getMedianOfLastFrames /*= true*/, bool useOldConvexityDefectMethod /*= false*/)
{
	int fingerCount = -1;

	if (getMedianOfLastFrames)
	{
		fingerCount = calculateMedianFingerCount( leftHand ? m_leftFingerCount : m_rightFingerCount);		
	}

	if (fingerCount == -1 && FubiCore::getInstance())
	{
		// No precalculations present or wanted, so calculate one instantly

		// Prepare image debug data
		FingerCountImageData* debugData = leftHand ? (&m_leftFingerCountImage) : (&m_rightFingerCountImage);
		FubiImageProcessing::releaseImage(debugData->image);
		debugData->image = 0x0;

		// Now get the finger count
		fingerCount = FubiImageProcessing::applyFingerCount(FubiCore::getInstance()->getSensor(), m_id, leftHand, 
							useOldConvexityDefectMethod, debugData);
	}

	return fingerCount;
}

void FubiUser::updateTrackingData(FubiISensor* sensor)
{
	if (sensor)
	{
		// First update tracking state
		m_isTracked = sensor->isTracking(m_id);

		if (sensor->hasNewTrackingData())
		{
			// Update timestamp
			m_lastTrackingData.timeStamp = m_currentTrackingData.timeStamp;
			m_currentTrackingData.timeStamp = Fubi::currentTime();

			// The other joints are only valid if the user is tracked
			if (m_isTracked)
			{
				// Get all joint positions for that user
				for (unsigned int j=0; j < SkeletonJoint::NUM_JOINTS; ++j)
				{
					// Backup old tracking info
					m_lastTrackingData.jointPositions[j] = m_currentTrackingData.jointPositions[j];
					m_lastTrackingData.localJointPositions[j] = m_currentTrackingData.localJointPositions[j];
					m_lastTrackingData.jointOrientations[j] = m_currentTrackingData.jointOrientations[j];
					m_lastTrackingData.localJointOrientations[j] = m_currentTrackingData.localJointOrientations[j];

					// And get new one
					sensor->getSkeletonJointData(m_id, (SkeletonJoint::Joint) j, m_currentTrackingData.jointPositions[j], m_currentTrackingData.jointOrientations[j]);
				}

				// Calculate local transformations out of the global ones
				calculateLocalTransformations();

				// Update body measurements (out of the local transformations)
				updateBodyMeasurements();
						
				// Immediately update the posture combination recognizers (Only if new joint data is here)
				updateCombinationRecognizers();

				// Check and update finger detection
				updateFingerCount();
			}
			else
			{
				// Only try to get the torso (should be independent of complete tracking)
				m_lastTrackingData.jointPositions[SkeletonJoint::TORSO] = m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];
				sensor->getSkeletonJointData(m_id, SkeletonJoint::TORSO, m_currentTrackingData.jointPositions[SkeletonJoint::TORSO], m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO]);
			}
		}
	}
}


void FubiUser::addNewTrackingData(Fubi::SkeletonJointPosition* positions,
	double timeStamp /*= -1*/, Fubi::SkeletonJointOrientation* orientations /*= 0*/, Fubi::SkeletonJointOrientation* localOrientations /*= 0*/)
{
	// Update timestamp
	m_lastTrackingData.timeStamp = m_currentTrackingData.timeStamp;
	if (timeStamp >= 0)
		m_currentTrackingData.timeStamp = timeStamp;
	else
		m_currentTrackingData.timeStamp = Fubi::currentTime();

	// Set new transformations
	for (unsigned int j=0; j < SkeletonJoint::NUM_JOINTS; ++j)
	{
		SkeletonJoint::Joint joint = (SkeletonJoint::Joint) j;
		// Backup old tracking info
		m_lastTrackingData.jointPositions[joint] = m_currentTrackingData.jointPositions[joint];
		m_lastTrackingData.jointOrientations[joint] = m_currentTrackingData.jointOrientations[joint];
		m_lastTrackingData.localJointOrientations[joint] = m_currentTrackingData.localJointOrientations[joint];
		// And get new one
		m_currentTrackingData.jointPositions[joint] = positions[j];
		if (orientations)
			m_currentTrackingData.jointOrientations[joint] = orientations[j];
		if (localOrientations)
			m_currentTrackingData.localJointOrientations[joint] = localOrientations[j];
	}

	if (orientations == 0)
	{
		// Try to calculate global orientations from position data (currently not very accurate)
		calculateGlobalOrientations();
	}

	if (localOrientations == 0)
	{
		// Calculate local orientations out of the global ones according to the OpenNI skeleton
		calculateLocalTransformations();
	}

	// Update body measurements (out of the local transformations)
	updateBodyMeasurements();
						
	// Immediately update the posture combination recognizers (Only if new joint data is here)
	updateCombinationRecognizers();

	// Check and update finger detection
	updateFingerCount();
}

void FubiUser::calculateGlobalOrientations()
{
	Vec3f vx;
	Vec3f vy;
	Vec3f temp;

	// Torso uses left-to-right-shoulder for x and torso-to-neck for y
	jointOrientationFromPositionsYX(m_currentTrackingData.jointPositions[SkeletonJoint::TORSO], m_currentTrackingData.jointPositions[SkeletonJoint::NECK], 
		m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER],
		m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO]);

	// Waist uses the same as torso
	m_currentTrackingData.jointOrientations[SkeletonJoint::WAIST] = m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO];

	// Neck uses left-to-right-shoulder for x and neck-to-head for y
	jointOrientationFromPositionsYX(m_currentTrackingData.jointPositions[SkeletonJoint::NECK], m_currentTrackingData.jointPositions[SkeletonJoint::HEAD],
		m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER],
		m_currentTrackingData.jointOrientations[SkeletonJoint::NECK]);

	// Head uses same as neck
	m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD] = m_currentTrackingData.jointOrientations[SkeletonJoint::NECK]; 
	
	// Shoulder uses elbow-to-shoulder for x
	jointOrientationFromPositionX(m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_ELBOW], m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER],
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_SHOULDER]);

	// Elbow uses hand-to-elbow for x
	jointOrientationFromPositionX(m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND], m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_ELBOW],
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ELBOW]);

	// Hand/wrist uses the same as elbow
	m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_WRIST] = m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_HAND] = m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ELBOW];

	// Hip uses knee-to-hip for y and left-to-right-hip for x
	jointOrientationFromPositionsYX(m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_KNEE], m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HIP],
		m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HIP], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HIP],
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_HIP]);

	// Knee users foot-to-knee for y 	
	jointOrientationFromPositionY(m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_FOOT], m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_KNEE],
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_KNEE]);

	// Foot/Ankle uses the same as knee
	m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ANKLE] = m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_FOOT] = m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_KNEE];


	// Shoulder uses shoulder-to-elbow for x
	jointOrientationFromPositionX(m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_ELBOW],
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_SHOULDER]);

	// Elbow uses elbow-to-hand for x
	jointOrientationFromPositionX(m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_ELBOW], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND],
	m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ELBOW]);

	// Hand/wrist uses the same as elbow
	m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_WRIST] = m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_HAND] = m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ELBOW];

	// Hip uses knee-to-hip for y and left-to-right-hip for x
	jointOrientationFromPositionsYX(m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_KNEE], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HIP],
		m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HIP], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HIP],
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_HIP]);

	// Knee users foot-to-knee for y 		
	jointOrientationFromPositionY(m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_FOOT], m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_KNEE],
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_KNEE]);

	// Foot/ankle uses the same as knee
	m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ANKLE] = m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_FOOT] = m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_KNEE];
}

void FubiUser::calculateLocalTransformations()
{
	// Calculate new relative orientations
	// Torso is the root, so the local orientation is the same as the global one
	m_currentTrackingData.localJointOrientations[SkeletonJoint::TORSO] = m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO];
	// Neck
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::NECK], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO], m_currentTrackingData.localJointOrientations[SkeletonJoint::NECK]);
	// Head
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::NECK], m_currentTrackingData.localJointOrientations[SkeletonJoint::HEAD]);
	// Nose
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::FACE_NOSE], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], m_currentTrackingData.localJointOrientations[SkeletonJoint::FACE_NOSE]);
	// Chin
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::FACE_CHIN], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], m_currentTrackingData.localJointOrientations[SkeletonJoint::FACE_CHIN]);
	// Forehead
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::FACE_FOREHEAD], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], m_currentTrackingData.localJointOrientations[SkeletonJoint::FACE_FOREHEAD]);
	// Left ear
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::FACE_LEFT_EAR], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], m_currentTrackingData.localJointOrientations[SkeletonJoint::FACE_LEFT_EAR]);
	// Right ear
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::FACE_RIGHT_EAR], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], m_currentTrackingData.localJointOrientations[SkeletonJoint::FACE_RIGHT_EAR]);
	// Left shoulder
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_SHOULDER], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::HEAD], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_SHOULDER]);
	// Left elbow
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ELBOW], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_SHOULDER], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_ELBOW]);
	// Left wrist
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_WRIST], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ELBOW], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_WRIST]);
	// Left hand
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_HAND], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_WRIST], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_HAND]);
	// Right shoulder
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_SHOULDER], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::NECK], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_SHOULDER]);
	// Right elbow
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ELBOW], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_SHOULDER], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_ELBOW]);
	// Right wrist
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_WRIST], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ELBOW], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_WRIST]);
	// Right hand
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_HAND], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_WRIST], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_HAND]);
	// Waist
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::WAIST], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO], m_currentTrackingData.localJointOrientations[SkeletonJoint::WAIST]);
	// Left hip
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_HIP], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::WAIST], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_HIP]);
	// Left knee
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_KNEE], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_HIP], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_KNEE]);
	// Left ankle
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ANKLE], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_KNEE], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_ANKLE]);
	// Left foot
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_FOOT], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::LEFT_ANKLE], m_currentTrackingData.localJointOrientations[SkeletonJoint::LEFT_FOOT]);
	// Right hip
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_HIP], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::WAIST], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_HIP]);
	// Right knee
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_KNEE], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_HIP], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_KNEE]);
	// Right ankle
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ANKLE], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_KNEE], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_ANKLE]);
	// Right foot
	Fubi::calculateLocalRotation(m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_FOOT], 
		m_currentTrackingData.jointOrientations[SkeletonJoint::RIGHT_ANKLE], m_currentTrackingData.localJointOrientations[SkeletonJoint::RIGHT_FOOT]);

	// Calculate local positions (removing the torso transformation=loc+rot from the position data)
	// Torso is the root, so the local pos is the same as the global one
	m_currentTrackingData.localJointPositions[SkeletonJoint::TORSO] = m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];
	const SkeletonJointPosition& torsoPos = m_currentTrackingData.jointPositions[SkeletonJoint::TORSO];
	const SkeletonJointOrientation& torsoRot = m_currentTrackingData.jointOrientations[SkeletonJoint::TORSO];
	for (unsigned int i = 0; i < SkeletonJoint::NUM_JOINTS; ++i)
	{
		if (i != SkeletonJoint::TORSO)
			Fubi::calculateLocalPosition(m_currentTrackingData.jointPositions[i], torsoPos, torsoRot, m_currentTrackingData.localJointPositions[i]);
	}
}

void FubiUser::updateCombinationRecognizers()
{
	// Update the posture combination recognizers
	for (unsigned int i=0; i < Fubi::Combinations::NUM_COMBINATIONS; ++i)
	{
		if (m_combinationRecognizers[i])
		{
			if (!m_combinationRecognizers[i]->isActive() && Fubi::getAutoStartCombinationRecognition((Fubi::Combinations::Combination)i))
			{
				// Reactivate combination recognizers that should already be active
				m_combinationRecognizers[i]->start();
			}
			m_combinationRecognizers[i]->update();
		}
	}
	std::map<std::string, CombinationRecognizer*>::iterator iter;
	std::map<std::string, CombinationRecognizer*>::iterator end = m_userDefinedCombinationRecognizers.end();
	for (iter = m_userDefinedCombinationRecognizers.begin(); iter != end; ++iter)
	{
		if (iter->second)
		{
			if (!iter->second->isActive() && Fubi::getAutoStartCombinationRecognition())
			{
				// Reactivate combination recognizers that should already be active
				iter->second->start();
			}
			iter->second->update();
		}
	}
}

void FubiUser::updateFingerCount()
{
	// Check and update finger detection
	if (m_lastLeftFingerDetection > -1
		&& (Fubi::currentTime() - m_lastLeftFingerDetection) > m_fingerTrackIntervall)
	{
		addFingerCount(getFingerCount(true, false, m_useConvexityDefectMethod), true);
	}
	if (m_lastRightFingerDetection > -1
		&& (Fubi::currentTime() - m_lastRightFingerDetection) > m_fingerTrackIntervall)
	{
		addFingerCount(getFingerCount(false, false, m_useConvexityDefectMethod), false);
	}
}

void FubiUser::updateBodyMeasurements()
{
	static const float filterFac = 0.1f;
	static const float updateIntervall = 0.5f;

	// Only once per second
	if (Fubi::currentTime()-m_lastBodyMeasurementUpdate > updateIntervall)
	{
		m_lastBodyMeasurementUpdate = Fubi::currentTime();

		// Select joints
		SkeletonJoint::Joint footToTake = SkeletonJoint::RIGHT_FOOT;
		SkeletonJoint::Joint kneeForFoot = SkeletonJoint::RIGHT_KNEE;
		if (m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_FOOT].m_confidence > m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_FOOT].m_confidence)
		{
			footToTake = SkeletonJoint::LEFT_FOOT;
			kneeForFoot = SkeletonJoint::LEFT_KNEE;
		}
		SkeletonJoint::Joint hipToTake = SkeletonJoint::RIGHT_HIP;
		SkeletonJoint::Joint kneeForHip = SkeletonJoint::RIGHT_KNEE;
		if (m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_KNEE].m_confidence > m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_KNEE].m_confidence)
		{
			hipToTake = SkeletonJoint::LEFT_HIP;
			kneeForHip = SkeletonJoint::LEFT_KNEE;
		}
		SkeletonJoint::Joint handToTake = SkeletonJoint::RIGHT_HAND;
		SkeletonJoint::Joint elbowForHand = SkeletonJoint::RIGHT_ELBOW;
		if (m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_confidence > m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND].m_confidence)
		{
			handToTake = SkeletonJoint::LEFT_HAND;
			elbowForHand = SkeletonJoint::LEFT_ELBOW;
		}
		SkeletonJoint::Joint shoulderToTake = SkeletonJoint::RIGHT_SHOULDER;
		SkeletonJoint::Joint elbowForShoulder = SkeletonJoint::RIGHT_ELBOW;
		if (m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_ELBOW].m_confidence > m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_ELBOW].m_confidence)
		{
			shoulderToTake = SkeletonJoint::LEFT_SHOULDER;
			elbowForShoulder = SkeletonJoint::LEFT_ELBOW;
		}

		// Body height
		//Add the neck-head distance to compensate for the missing upper head part
		SkeletonJointPosition headEnd(m_currentTrackingData.jointPositions[SkeletonJoint::HEAD]);
		headEnd.m_position = headEnd.m_position + (headEnd.m_position-m_currentTrackingData.jointPositions[SkeletonJoint::NECK].m_position);
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[footToTake],
			headEnd, m_bodyMeasurements[BodyMeasurement::BODY_HEIGHT], filterFac);

		// Torso height
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[SkeletonJoint::WAIST],
			m_currentTrackingData.jointPositions[SkeletonJoint::NECK], m_bodyMeasurements[BodyMeasurement::TORSO_HEIGHT],filterFac);

		// Shoulder width
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER],
			m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER], m_bodyMeasurements[BodyMeasurement::SHOULDER_WIDTH],filterFac);

		// Hip width
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HIP],
			m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HIP], m_bodyMeasurements[BodyMeasurement::HIP_WIDTH],filterFac);

		// Arm lengths
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[shoulderToTake],
			m_currentTrackingData.jointPositions[elbowForShoulder], m_bodyMeasurements[BodyMeasurement::UPPER_ARM_LENGTH],filterFac);
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[handToTake],
			m_currentTrackingData.jointPositions[elbowForHand], m_bodyMeasurements[BodyMeasurement::LOWER_ARM_LENGTH],filterFac);
		m_bodyMeasurements[BodyMeasurement::ARM_LENGTH].m_dist = m_bodyMeasurements[BodyMeasurement::LOWER_ARM_LENGTH].m_dist + m_bodyMeasurements[BodyMeasurement::UPPER_ARM_LENGTH].m_dist;
		m_bodyMeasurements[BodyMeasurement::ARM_LENGTH].m_confidence = minf(m_bodyMeasurements[BodyMeasurement::LOWER_ARM_LENGTH].m_confidence, m_bodyMeasurements[BodyMeasurement::UPPER_ARM_LENGTH].m_confidence);

		// Leg lengths
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[hipToTake],
			m_currentTrackingData.jointPositions[kneeForHip], m_bodyMeasurements[BodyMeasurement::UPPER_LEG_LENGTH],filterFac);
		Fubi::calculateBodyMeasurement(m_currentTrackingData.jointPositions[footToTake],
			m_currentTrackingData.jointPositions[kneeForFoot], m_bodyMeasurements[BodyMeasurement::LOWER_LEG_LENGTH],filterFac);
		m_bodyMeasurements[BodyMeasurement::LEG_LENGTH].m_dist = m_bodyMeasurements[BodyMeasurement::LOWER_LEG_LENGTH].m_dist + m_bodyMeasurements[BodyMeasurement::UPPER_LEG_LENGTH].m_dist;
		m_bodyMeasurements[BodyMeasurement::LEG_LENGTH].m_confidence = minf(m_bodyMeasurements[BodyMeasurement::LOWER_LEG_LENGTH].m_confidence, m_bodyMeasurements[BodyMeasurement::UPPER_LEG_LENGTH].m_confidence);
	}
}

void FubiUser::reset()
{
	m_isTracked = false;
	m_inScene = false;
	m_id = 0;
	m_lastRightFingerDetection = -1;
	m_lastLeftFingerDetection = -1;
	m_lastBodyMeasurementUpdate = 0;
}