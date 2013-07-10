// ****************************************************************************************
//
// Fubi Sensor Interface
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************
#pragma once

#include "FubiUtils.h"

// The Fubi Sensor interface offers depth/rgb/ir image streams and user tracking data
class FubiISensor
{
public:
	virtual ~FubiISensor() {}

	// Update should be called once per frame for the sensor to update its streams and tracking data
	virtual void update() = 0;

	// Get the ids of all currently valid users: Ids will be stored in userIDs (if not 0x0), returns the number of valid users
	virtual unsigned short getUserIDs(unsigned int* userIDs) = 0;

	// Check if the sensor has new tracking data available
	virtual bool hasNewTrackingData() = 0;

	// Check if that user with the given id is tracked by the sensor
	virtual bool isTracking(unsigned int id) = 0;

	// Get the current joint position and orientation of one user
	virtual void getSkeletonJointData(unsigned int id, Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJointPosition& position, Fubi::SkeletonJointOrientation& orientation) = 0;

	// Get the current tracked face points of a user
	virtual int getFacePoints(unsigned int id, Fubi::Vec3f* pointArray121, bool projected2DPoints = false, Fubi::Vec3f* triangleIndexArray121 = 0x0)
	{
		return -1;
	}

	// Get Stream data
	virtual const unsigned short* getDepthData() = 0;
	virtual const unsigned char* getRgbData()
	{
		return 0x0;
	}
	virtual const unsigned short* getIrData()
	{
		return 0x0;
	}
	virtual const unsigned short* getUserLabelData()
	{
		return 0x0;
	}

	// Init with options for streams and tracking
	virtual bool initWithOptions(const Fubi::SensorOptions& options)
	{
		Fubi_logWrn("initWithOptions not supported by this sensor!\n");
		return false;
	}

	// Init with a sensor specific xml file for options
	virtual bool initFromXml(const char* xmlPath, Fubi::SkeletonTrackingProfile::Profile profile = Fubi::SkeletonTrackingProfile::ALL,
		bool mirrorStreams = true, float smoothing = 0)
	{
		Fubi_logWrn("initFromXml not supported by this sensor!\n");
		return false;
	}

	// Resets the tracking of a users
	virtual void resetTracking(unsigned int id)
	{
		Fubi_logWrn("resetTracking not supported by this sensor!\n");
	}

	// Get the floor plane
	virtual Fubi::Plane getFloor()
	{
		Fubi_logWrn("getFloor not supported by this sensor!\n");
		return Fubi::Plane();
	}

	// Return real world to projective according to openni sensor
	virtual Fubi::Vec3f realWorldToProjective(const Fubi::Vec3f& realWorldVec) = 0;

	// TODO: setOptions?

	// Get Options
	const Fubi::StreamOptions& getDepthOptions() { return m_options.m_depthOptions; }
	const Fubi::StreamOptions& getRgbOptions()  { return m_options.m_rgbOptions; }
	const Fubi::StreamOptions& getIROptions()  { return m_options.m_irOptions; }
	const Fubi::SensorOptions& getOptions() { return m_options; }

	Fubi::SensorType::Type getType() { return m_options.m_type; }

protected:
	Fubi::SensorOptions m_options;

};