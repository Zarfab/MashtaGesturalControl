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
#include "RightHandOutRecognizer.h"

using namespace Fubi;

Fubi::RecognitionResult::Result RightHandOutRecognizer::recognizeOn(FubiUser* user)
{
	const SkeletonJointPosition& rightHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND];
	const SkeletonJointPosition& rightShoulder = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER];
	if (rightHand.m_confidence >= m_minConfidence && rightShoulder.m_confidence >= m_minConfidence)
	{
		if (abs(rightHand.m_position.y-rightShoulder.m_position.y) < 300.0f
			&& rightHand.m_position.x > rightShoulder.m_position.x + 350.0f)
			return Fubi::RecognitionResult::RECOGNIZED;
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}