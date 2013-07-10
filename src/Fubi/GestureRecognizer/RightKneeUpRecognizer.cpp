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
#include "RightKneeUpRecognizer.h"

using namespace Fubi;

RecognitionResult::Result RightKneeUpRecognizer::recognizeOn(FubiUser* user)
{
	const SkeletonJointPosition& rightKnee = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_KNEE];
	const SkeletonJointPosition& rightHip = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HIP];
	if (rightKnee.m_confidence >= m_minConfidence && rightHip.m_confidence >= m_minConfidence)
	{
		if (abs(rightKnee.m_position.y - rightHip.m_position.y) < 250.0f)
			return RecognitionResult::RECOGNIZED;
	}
	else
		return RecognitionResult::TRACKING_ERROR;
	return RecognitionResult::NOT_RECOGNIZED;
}