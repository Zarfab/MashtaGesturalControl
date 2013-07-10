// ****************************************************************************************
//
// Fubi Linear Movement Recognizer
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#include "LinearMovementRecognizer.h"


using namespace Fubi;

LinearMovementRecognizer::LinearMovementRecognizer(Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJoint::Joint relJoint, 
		const Fubi::Vec3f& direction, float minVel, float maxVel /*= Fubi::Math::MaxFloat*/, bool useLocalPos /*= false*/,
		float minConfidence /*= -1.0f*/, float maxAngleDiff /*= 45.0f*/, 
		bool useOnlyCorrectDirectionComponent /*= true*/)
	: m_joint(joint), m_relJoint(relJoint), m_useRelJoint(true),
	  m_minVel(minVel), m_maxVel(maxVel), m_useLocalPos(useLocalPos),
	  m_maxAngleDiff(maxAngleDiff), m_useOnlyCorrectDirectionComponent(useOnlyCorrectDirectionComponent),
	  IGestureRecognizer(false, minConfidence)
{
	m_directionValid = direction.length() > Math::Epsilon;
	if (m_directionValid)
		m_direction = direction.normalized();
}

LinearMovementRecognizer::LinearMovementRecognizer(Fubi::SkeletonJoint::Joint joint,
		const Fubi::Vec3f& direction, float minVel, float maxVel /*= Fubi::Math::MaxFloat*/,
		bool useLocalPos /*= false*/, float minConfidence /*= -1.0f*/, float maxAngleDiff /*= 45.0f*/, 
		bool useOnlyCorrectDirectionComponent /*= true*/)
	: m_joint(joint), m_useRelJoint(false),
	m_minVel(minVel), m_maxVel(maxVel), m_useLocalPos(useLocalPos),
	m_maxAngleDiff(maxAngleDiff), m_useOnlyCorrectDirectionComponent(useOnlyCorrectDirectionComponent),
	IGestureRecognizer(false, minConfidence)
{
	m_directionValid = direction.length() > Math::Epsilon;
	if (m_directionValid)
		m_direction = direction.normalized();

	if (minConfidence >= 0)
		m_minConfidence = minConfidence;
}

Fubi::RecognitionResult::Result LinearMovementRecognizer::recognizeOn(FubiUser* user)
{
	Fubi::RecognitionResult::Result result = Fubi::RecognitionResult::NOT_RECOGNIZED;
	
	if (user != 0x0)
	{
		// Get joint positions
		SkeletonJointPosition* joint = &(user->m_currentTrackingData.jointPositions[m_joint]);
		SkeletonJointPosition* lastJoint = &(user->m_lastTrackingData.jointPositions[m_joint]);
		if (m_useLocalPos)
		{
			joint = &(user->m_currentTrackingData.localJointPositions[m_joint]);
			lastJoint = &(user->m_lastTrackingData.localJointPositions[m_joint]);
		}

		// Check confidence
		if (joint->m_confidence >= m_minConfidence && lastJoint->m_confidence >= m_minConfidence)
		{
			bool relJointsValid = false;

			// Calculate relative vector of current and last frame
			Vec3f vector(Fubi::Math::NO_INIT);
			Vec3f lastVector(Fubi::Math::NO_INIT);
			if (m_useRelJoint)
			{
				// Using the other joint
				SkeletonJointPosition* relJoint = &(user->m_currentTrackingData.jointPositions[m_relJoint]);
				SkeletonJointPosition* lastRelJoint = &(user->m_lastTrackingData.jointPositions[m_relJoint]);
				if (m_useLocalPos)
				{
					relJoint = &(user->m_currentTrackingData.localJointPositions[m_relJoint]);
					lastRelJoint = &(user->m_lastTrackingData.localJointPositions[m_relJoint]);
				}
				relJointsValid = relJoint->m_confidence >= m_minConfidence && lastRelJoint->m_confidence >= m_minConfidence;
				if(relJointsValid)
				{
					vector = joint->m_position - relJoint->m_position;
					lastVector = lastJoint->m_position -lastRelJoint->m_position;
				}
			}
			else
			{
				// Absolute values (relative to Kinect position)
				relJointsValid = true;
				vector = joint->m_position;
				lastVector =lastJoint->m_position;
			}
	
		
			if (relJointsValid)
			{
				// Get the difference between both vectors and the time
				Vec3f diffVector = vector - lastVector;
				float diffTime = clamp(float(user->m_currentTrackingData.timeStamp - user->m_lastTrackingData.timeStamp), Math::Epsilon, Math::MaxFloat);
	
				float vel = 0;
				float angleDiff = 0;
				if (m_directionValid)
				{
					if (m_useOnlyCorrectDirectionComponent)
					{
						// Weight the vector components according to the given direction
						// Apply the direction stretched to the same length on the vector
						// Components in the correct direction will result in a positive value
						// Components in the wrong direction have a negative value
						Vec3f dirVector = diffVector * (m_direction * diffVector.length());
			
						// Build the sum of the weighted and signed components
						float sum = dirVector.x + dirVector.y + dirVector.z;

						// Calcluate the velocity (if there are too many negative values it may be less then zero)
						vel = (sum <= 0) ? (-sqrt(-sum) / diffTime) : (sqrt(sum) / diffTime);
					}
					else
						// calculate the velocity directly from the current vector
						vel = diffVector.length() / diffTime;

					// Additionally check the angle difference
					angleDiff = radToDeg(acosf(diffVector.dot(m_direction) / (diffVector.length() * m_direction.length())));
				}
				else
				{
					// No direction given so check for movement speed in any direction
					vel = diffVector.length() / diffTime;
				}

				// Check if velocity is in between the boundaries
				if (vel >= m_minVel && vel <= m_maxVel && angleDiff <= m_maxAngleDiff)
					result = RecognitionResult::RECOGNIZED;

				//if (/*!recognized && */abs(vel) > 200)
				//{
				//	if (m_maxVel > 10000.0f)
				//	{
				//		Fubi_logInfo("Lin Gesture rec: vel=%4.0f <= %4.0f <= INF recognized=%s\n", 
				//		  m_minVel, vel, (result == RecognitionResult::RECOGNIZED) ? "true" : "false");
				//	}
				//	else
				//		Fubi_logInfo("Lin Gesture rec: vel=%4.0f <= %4.0f <= %4.0f recognized=%s\n", 
				//		m_minVel, vel, m_maxVel, (result == RecognitionResult::RECOGNIZED) ? "true" : "false");
				//	/*diffVector.normalize();

				//	Fubi_logInfo("Lin Gesture rec: Hand.z=%.3f, targetDir=%.3f/%.3f/%.3f \n\t\tactualDir=%.3f/%.3f/%.3f vel=%.0f/%.0f recognized=%s\n", 
				//		joint.m_position.z,
				//		m_direction.x, m_direction.y, m_direction.z, 
				//		diffVector.x, diffVector.y, diffVector.z,
				//		vel, m_minVel, recognized ? "true" : "false");*/
				//}
			}
			else
				result = Fubi::RecognitionResult::TRACKING_ERROR;
		}
		else
			result = Fubi::RecognitionResult::TRACKING_ERROR;
	}

	return result;
}