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

#pragma once

#include "IGestureRecognizer.h"

class LinearMovementRecognizer : public IGestureRecognizer
{
public:
	LinearMovementRecognizer(Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJoint::Joint relJoint, 
		const Fubi::Vec3f& direction, float minVel, float maxVel = Fubi::Math::MaxFloat,
		bool useLocalPos = false, float minConfidence = -1.0f,
		float maxAngleDiff = 45.0f, bool useOnlyCorrectDirectionComponent = true);
	LinearMovementRecognizer(Fubi::SkeletonJoint::Joint joint, const Fubi::Vec3f& direction,
		float minVel, float maxVel = Fubi::Math::MaxFloat,
		bool useLocalPos = false, float minConfidence = -1.0f,
		float maxAngleDiff = 45.0f, bool useOnlyCorrectDirectionComponent = true);

	virtual ~LinearMovementRecognizer() {}

	virtual Fubi::RecognitionResult::Result recognizeOn(FubiUser* user);

	virtual IGestureRecognizer* clone() { return new LinearMovementRecognizer(*this); }

private:
	Fubi::SkeletonJoint::Joint m_joint;
	Fubi::SkeletonJoint::Joint m_relJoint;
	Fubi::Vec3f m_direction;
	bool m_directionValid;
	bool m_useOnlyCorrectDirectionComponent;
	float m_minVel;
	float m_maxVel;
	bool m_useRelJoint;
	bool m_useLocalPos;
	float m_maxAngleDiff;
};