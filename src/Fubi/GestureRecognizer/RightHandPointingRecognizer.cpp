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
#include "RightHandPointingRecognizer.h"

using namespace Fubi;


Fubi::RecognitionResult::Result  RightHandPointingRecognizer::recognizeOn(FubiUser* user)
{
	const SkeletonJointPosition& rightHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND];
	const SkeletonJointPosition& rightShoulder = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER];
	const SkeletonJointPosition& rightElbow = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_ELBOW];
	if (rightHand.m_confidence >= m_minConfidence && rightShoulder.m_confidence >= m_minConfidence && rightElbow.m_confidence >= m_minConfidence)
	{
		Vec3f origin = rightShoulder.m_position;
		Vec3f dir = rightHand.m_position-origin;
		float dist = distancePointToRay(rightElbow.m_position, origin, dir);

		bool armStretched = dist >= 0.0f && dist < 120.0f;
		bool notDownwards = dir.y > 0 || sqrtf(dir.x*dir.x + dir.z*dir.z) > 300.0f;
		//Fubi_logInfo("distToRay: %.0f\n", dist);

		if (armStretched && notDownwards)
			return Fubi::RecognitionResult::RECOGNIZED;
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}