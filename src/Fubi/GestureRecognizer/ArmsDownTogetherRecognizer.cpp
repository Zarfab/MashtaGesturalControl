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
#include "ArmsDownTogetherRecognizer.h"

using namespace Fubi;



Fubi::RecognitionResult::Result ArmsDownTogetherRecognizer::recognizeOn(FubiUser* user)
{
	bool recognized = false;
	const SkeletonJointPosition& rightHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND];
	const SkeletonJointPosition& rightShoulder = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER];
	const SkeletonJointPosition& leftHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND];
	const SkeletonJointPosition& leftShoulder = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER];
	if (rightHand.m_confidence >= m_minConfidence && rightShoulder.m_confidence >= m_minConfidence
		&& leftHand.m_confidence >= m_minConfidence && leftShoulder.m_confidence >= m_minConfidence)
	{
		Vec3f rightArm = rightHand.m_position - rightShoulder.m_position;
		Vec3f leftArm = leftHand.m_position - leftShoulder.m_position;
		float handDist = (leftHand.m_position - rightHand.m_position).length();
		bool littleX =  (rightArm.x < -50 && rightArm.x > -175) && (leftArm.x > 50 && leftArm.x < 175);
		bool littleZ =  (rightArm.z < 125 && rightArm.z > -300) && (leftArm.z < 125 && leftArm.z > -300);
		bool YDown = rightArm.y < -300 && leftArm.y < -300;
		recognized = littleX && littleZ && YDown && handDist < 150;
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;
	return recognized ? Fubi::RecognitionResult::RECOGNIZED : Fubi::RecognitionResult::NOT_RECOGNIZED;
}