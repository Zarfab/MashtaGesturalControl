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
#include "HandsFrontTogetherRecognizer.h"

using namespace Fubi;



Fubi::RecognitionResult::Result HandsFrontTogetherRecognizer::recognizeOn(FubiUser* user)
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
		bool littleX =  (rightArm.x < -25 && rightArm.x > -275) && (leftArm.x > 25 && leftArm.x < 275);
		bool moreZ =  rightArm.z < -300 && leftArm.z < -300;
		recognized = littleX && moreZ && handDist < 150;
		/*if (!recognized)
		{
			Fubi_logInfo("HandsFrontFailed: rightArm.x: %.0f, leftArm.x: %.0f, rightArm.z: %.0f, leftArm.z: %.0f\n", rightArm.x, leftArm.x, rightArm.z, leftArm.z);
		}*/
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;
	return recognized ? Fubi::RecognitionResult::RECOGNIZED : Fubi::RecognitionResult::NOT_RECOGNIZED;
}