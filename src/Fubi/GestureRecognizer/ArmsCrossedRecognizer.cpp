// ****************************************************************************************
//
// Posture Recognizers
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************
#include "ArmsCrossedRecognizer.h"

using namespace Fubi;


Fubi::RecognitionResult::Result ArmsCrossedRecognizer::recognizeOn(FubiUser* user)
{
	bool recognized = false;
	const SkeletonJointPosition& rightElbow = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_ELBOW];
	const SkeletonJointPosition& rightShoulder = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER];
	const SkeletonJointPosition& rightHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND];

	const SkeletonJointPosition& leftHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND];
	const SkeletonJointPosition& leftElbow = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_ELBOW];
	const SkeletonJointPosition& leftShoulder = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER];
	
	if (rightHand.m_confidence >= m_minConfidence && rightElbow.m_confidence >= m_minConfidence
		&& leftHand.m_confidence >= m_minConfidence && leftElbow.m_confidence >= m_minConfidence
		&& leftShoulder.m_confidence >= m_minConfidence && rightShoulder.m_confidence >= m_minConfidence)
	{
		Vec3f rightArm = rightHand.m_position - rightElbow.m_position;
		Vec3f leftArm = leftHand.m_position - leftElbow.m_position;
		Vec3f rightWholeArm = rightElbow.m_position - rightShoulder.m_position;
		Vec3f leftWholeArm = leftElbow.m_position - leftShoulder.m_position;
		bool littleY = abs(rightArm.y) < 175 && abs(leftArm.y) < 175;
		bool littleZ = abs(rightArm.z) < 175 && abs(leftArm.z) < 175;
		bool highXInsideDirection = rightArm.x < -220 && leftArm.x > 220;
		bool armsDown = rightWholeArm.y < 0 && leftWholeArm.y < 0; 
		recognized = littleY && littleZ && highXInsideDirection && armsDown;
	}
	else return RecognitionResult::TRACKING_ERROR;
	return (recognized ? RecognitionResult::RECOGNIZED : RecognitionResult::NOT_RECOGNIZED);
}