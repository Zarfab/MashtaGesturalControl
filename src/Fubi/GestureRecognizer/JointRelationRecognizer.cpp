// ****************************************************************************************
//
// Joint Relation Recognizers
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************
#include "JointRelationRecognizer.h"

using namespace Fubi;

JointRelationRecognizer::JointRelationRecognizer(Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJoint::Joint relJoint, 
	const Fubi::Vec3f& minValues /*= Fubi::Vec3f(-Fubi::Math::MaxFloat,-Fubi::Math::MaxFloat, -Fubi::Math::MaxFloat)*/, 
	const Fubi::Vec3f& maxValues /*= Fubi::Vec3f(Fubi::Math::MaxFloat, Fubi::Math::MaxFloat, Fubi::Math::MaxFloat)*/, 
	float minDistance /*= 0*/, 
	float maxDistance /*= Fubi::Math::MaxFloat*/,
	bool useLocalPositions /*=false*/,
	float minConfidence /*= -1.0f*/,
	Fubi::BodyMeasurement::Measurement measuringUnit /*= Fubi::BodyMeasurement::NUM_MEASUREMENTS*/)
	: m_joint(joint), m_relJoint(relJoint),
	  m_minValues(minValues), m_maxValues(maxValues), m_minDistance(minDistance), m_maxDistance(maxDistance), 
	  m_useLocalPositions(useLocalPositions),
	  IGestureRecognizer(false, minConfidence),
	  m_measuringUnit(measuringUnit)
{
}

Fubi::RecognitionResult::Result JointRelationRecognizer::recognizeOn(FubiUser* user)
{
	bool recognized = false;
	
	SkeletonJointPosition* joint = &(user->m_currentTrackingData.jointPositions[m_joint]);
	if (m_useLocalPositions)
		joint = &(user->m_currentTrackingData.localJointPositions[m_joint]);
	
	if (joint->m_confidence >= m_minConfidence)
	{
		bool vecValid = false;
		Vec3f vector(Fubi::Math::NO_INIT);
		

		if (m_relJoint != Fubi::SkeletonJoint::NUM_JOINTS)
		{
			SkeletonJointPosition* relJoint = &(user->m_currentTrackingData.jointPositions[m_relJoint]);
			if (m_useLocalPositions)
				relJoint = &(user->m_currentTrackingData.localJointPositions[m_relJoint]);

			vecValid = relJoint->m_confidence >= m_minConfidence;
			if(vecValid)
			{
				vector = joint->m_position - relJoint->m_position;
			}
		}
		else
		{
			vector = joint->m_position;
			vecValid = true;
		}
			
		if (vecValid && m_measuringUnit != BodyMeasurement::NUM_MEASUREMENTS)
		{
			BodyMeasurementDistance& measure = user->m_bodyMeasurements[m_measuringUnit];
			if (measure.m_confidence >= m_minConfidence	&& measure.m_dist > Math::Epsilon)
				vector /= measure.m_dist;
			else
				vecValid = false;
		}

		if (vecValid)
		{			

			float distance = vector.length();
		
			bool xInRange = vector.x >= m_minValues.x && vector.x <= m_maxValues.x;
			bool yInRange = vector.y >= m_minValues.y && vector.y <= m_maxValues.y;
			bool zInRange = vector.z >= m_minValues.z && vector.z <= m_maxValues.z;

			bool distInRange = distance >= m_minDistance && distance <= m_maxDistance;

			recognized = xInRange && yInRange && zInRange && distInRange;
		}
		else
			return Fubi::RecognitionResult::TRACKING_ERROR;
	}
	else
		return Fubi::RecognitionResult::TRACKING_ERROR;

	return recognized ? Fubi::RecognitionResult::RECOGNIZED : Fubi::RecognitionResult::NOT_RECOGNIZED;
}