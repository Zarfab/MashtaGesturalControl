// ****************************************************************************************
//
// Joint Orientation Recognizers
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************
#include "JointOrientationRecognizer.h"

using namespace Fubi;

JointOrientationRecognizer::JointOrientationRecognizer(Fubi::SkeletonJoint::Joint joint, const Fubi::Vec3f& minValues /*= Fubi::Vec3f(-180.0f,-180.0f,-180.0f)*/, 
		const Fubi::Vec3f& maxValues /*= Fubi::Vec3f(180.0f, 180.0f, 180.0f)*/, bool useLocalOrientation /*= true*/, float minConfidence /*= -1.0f*/)
	: m_joint(joint), m_minValues(minValues), m_maxValues(maxValues), m_useLocalOrientations(useLocalOrientation), IGestureRecognizer(false, minConfidence)
{
	normalizeRotationVec(m_minValues);
	normalizeRotationVec(m_maxValues);
}

Fubi::RecognitionResult::Result JointOrientationRecognizer::recognizeOn(FubiUser* user)
{
	bool recognized = false;
	bool foundRotation = false;
	Vec3f orient(Fubi::Math::NO_INIT);

	if (m_useLocalOrientations)
	{
		const SkeletonJointOrientation& joint = user->m_currentTrackingData.localJointOrientations[m_joint];
		if (joint.m_confidence >= m_minConfidence)
		{
			orient = joint.m_orientation.getRot();
			foundRotation = true;
		}
	}
	else
	{
		const SkeletonJointOrientation& joint = user->m_currentTrackingData.jointOrientations[m_joint];
		if (joint.m_confidence >= m_minConfidence)
		{
			orient = joint.m_orientation.getRot();
			foundRotation = true;
		}
	}

	if (foundRotation)
	{
		// Note the special case when a min value is larger than the max value
		// In this case the -180/+180 rotation is between min and max
		// The || operator works as the min values and max values are always normalized to [-180;180]
		bool xInRange = (m_minValues.x <= m_maxValues.x)
			? (orient.x >= m_minValues.x && orient.x <= m_maxValues.x)
			: (orient.x >= m_minValues.x || orient.x <= m_maxValues.x);
		bool yInRange = (m_minValues.y <= m_maxValues.y)
			? (orient.y >= m_minValues.y && orient.y <= m_maxValues.y)
			: (orient.y >= m_minValues.y || orient.y <= m_maxValues.y);
		bool zInRange = (m_minValues.z <= m_maxValues.z)
			? (orient.z >= m_minValues.z && orient.z <= m_maxValues.z)
			: (orient.z >= m_minValues.z || orient.z <= m_maxValues.z);
		recognized = xInRange && yInRange && zInRange;
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;

	return recognized ? Fubi::RecognitionResult::RECOGNIZED : Fubi::RecognitionResult::NOT_RECOGNIZED;
}