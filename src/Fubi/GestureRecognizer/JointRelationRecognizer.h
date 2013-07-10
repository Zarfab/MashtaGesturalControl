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

#pragma once

#include "IGestureRecognizer.h"

class JointRelationRecognizer : public IGestureRecognizer
{
public:
	// +-MaxFloat are the default values, as they represent no restriction
	JointRelationRecognizer(Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJoint::Joint relJoint,
		const Fubi::Vec3f& minValues = Fubi::Vec3f(-Fubi::Math::MaxFloat,-Fubi::Math::MaxFloat, -Fubi::Math::MaxFloat), 
		const Fubi::Vec3f& maxValues = Fubi::Vec3f(Fubi::Math::MaxFloat, Fubi::Math::MaxFloat, Fubi::Math::MaxFloat),
		float minDistance = 0, 
		float maxDistance = Fubi::Math::MaxFloat,
		bool useLocalPositions = false,
		float minConfidence = -1.0f,
		Fubi::BodyMeasurement::Measurement measuringUnit = Fubi::BodyMeasurement::NUM_MEASUREMENTS);

	virtual ~JointRelationRecognizer() {}

	virtual Fubi::RecognitionResult::Result recognizeOn(FubiUser* user);
	virtual IGestureRecognizer* clone() { return new JointRelationRecognizer(*this); }

private:
	Fubi::SkeletonJoint::Joint m_joint;
	Fubi::SkeletonJoint::Joint m_relJoint;
	Fubi::Vec3f m_minValues, m_maxValues;
	float m_minDistance;
	float m_maxDistance;
	bool m_useLocalPositions;
	Fubi::BodyMeasurement::Measurement m_measuringUnit;
};