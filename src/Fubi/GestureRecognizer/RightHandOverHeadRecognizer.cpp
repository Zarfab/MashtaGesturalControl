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
#include "RightHandOverHeadRecognizer.h"

using namespace Fubi;

Fubi::RecognitionResult::Result RightHandOverHeadRecognizer::recognizeOn(FubiUser* user)
{
	const SkeletonJointPosition& rightHand = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND];
	const SkeletonJointPosition& head = user->m_currentTrackingData.jointPositions[SkeletonJoint::HEAD];
	if (rightHand.m_confidence >= m_minConfidence && head.m_confidence >= m_minConfidence)
	{
		if (rightHand.m_position.y > head.m_position.y)
			return Fubi::RecognitionResult::RECOGNIZED;
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;
	return Fubi::RecognitionResult::NOT_RECOGNIZED;
}