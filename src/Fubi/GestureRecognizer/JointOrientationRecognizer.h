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

#pragma once

#include "IGestureRecognizer.h"

class JointOrientationRecognizer : public IGestureRecognizer
{
public:
	// orientation values in degrees
	JointOrientationRecognizer(Fubi::SkeletonJoint::Joint joint, const Fubi::Vec3f& minValues = Fubi::Vec3f(-180.0f,-180.0f,-180.0f), 
		const Fubi::Vec3f& maxValues = Fubi::Vec3f(180.0f, 180.0f, 180.0f), bool useLocalOrientation = true, float minConfidence = -1.0f);

	virtual ~JointOrientationRecognizer() {}

	virtual Fubi::RecognitionResult::Result recognizeOn(FubiUser* user);
	virtual IGestureRecognizer* clone() { return new JointOrientationRecognizer(*this); }

private:
	Fubi::SkeletonJoint::Joint m_joint;
	Fubi::Vec3f m_minValues, m_maxValues;
	bool m_useLocalOrientations;
};