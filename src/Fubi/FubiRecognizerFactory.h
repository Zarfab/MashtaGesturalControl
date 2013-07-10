// ****************************************************************************************
//
// Fubi Recognizer Factory
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once

// Common interface for recognizers
#include "GestureRecognizer/IGestureRecognizer.h"

#include "Fubi.h"

using namespace std;

namespace Fubi
{
	// Create a static posture recognizer
	IGestureRecognizer* createRecognizer(Postures::Posture postureID);

	// Create a joint relation recognizer
	IGestureRecognizer* createRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint,
		const Fubi::Vec3f& minValues = DefaultMinVec, 
		const Fubi::Vec3f& maxValues = DefaultMaxVec, 
		float minDistance = 0, 
		float maxDistance = Fubi::Math::MaxFloat,
		bool useLocalPositions = false,
		float minConfidence = -1.0f,
		Fubi::BodyMeasurement::Measurement measuringUnit = Fubi::BodyMeasurement::NUM_MEASUREMENTS);

	// Create a joint orientation recognizer
	IGestureRecognizer* createRecognizer(SkeletonJoint::Joint joint,
		const Fubi::Vec3f& minValues = Fubi::Vec3f(-180.0f, -180.0f, -180.0f), 
		const Fubi::Vec3f& maxValues = Fubi::Vec3f(180.0f, 180.0f, 180.0f),
		bool useLocalOrientations = true,
		float minConfidence = -1.0f);

	// Create a linear movement recognizer
	IGestureRecognizer* createRecognizer(SkeletonJoint::Joint joint, SkeletonJoint::Joint relJoint,
		const Fubi::Vec3f& direction, 
		float minVel, float maxVel = Fubi::Math::MaxFloat,
		bool useLocalPositions = false,
		float minConfidence = -1.0f,
		float maxAngleDiff = 45.0f, 
		bool useOnlyCorrectDirectionComponent = true);
	IGestureRecognizer* createRecognizer(SkeletonJoint::Joint joint,
		const Fubi::Vec3f& direction, 
		float minVel, float maxVel = Fubi::Math::MaxFloat,
		bool useLocalPositions = false,
		float minConfidence = -1.0f,
		float maxAngleDiff = 45.0f, 
		bool useOnlyCorrectDirectionComponent = true);

	// Create a finger count recognizer
	IGestureRecognizer* createRecognizer(Fubi::SkeletonJoint::Joint handJoint = Fubi::SkeletonJoint::RIGHT_HAND,
		unsigned int minFingers = 0, unsigned int maxFingers = 5,
		float minConfidence = -1.0f, bool useMedianCalculation = false);

	// Create a posture combination recognizer
	CombinationRecognizer* createRecognizer(FubiUser* user, Combinations::Combination postureID);
}
