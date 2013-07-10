// ****************************************************************************************
//
// Fubi Image Processing
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once

// General includes
#include "FubiUtils.h"

// Sensor interface for getting stream data
#include "FubiISensor.h"

class FubiImageProcessing
{
public:
	// Get color of a user in the enhanced depth image
	static void getColorForUserID(unsigned int id, float& r, float& g, float& b);
	
	// Draw an image into the given buffer, returns true if succesful
	static bool getImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageType::Type type, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, 
		unsigned int renderOptions = (Fubi::RenderOptions::Shapes | Fubi::RenderOptions::Skeletons | Fubi::RenderOptions::UserCaptions),
		Fubi::DepthImageModification::Modification depthModifications = Fubi::DepthImageModification::UseHistogram,
        unsigned int userId = 0, Fubi::SkeletonJoint::Joint jointOfInterest = Fubi::SkeletonJoint::NUM_JOINTS,
        FubiUserGesture current_gestures = FubiUserGesture() );

	// Save a picture of one user (or the whole scene if userId = 0)
	static bool saveImage(FubiISensor* sensor, const char* fileName, int jpegQuality, 
		Fubi::ImageType::Type type, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth,
		unsigned int renderOptions = (Fubi::RenderOptions::Shapes | Fubi::RenderOptions::Skeletons | Fubi::RenderOptions::UserCaptions),
		Fubi::DepthImageModification::Modification depthModifications = Fubi::DepthImageModification::UseHistogram,
		unsigned int userId = 0, Fubi::SkeletonJoint::Joint jointOfInterest = Fubi::SkeletonJoint::NUM_JOINTS);

	// Applies a fingercount calculation for one hand of one user
	static int applyFingerCount(FubiISensor* sensor, unsigned int userID, bool leftHand = false, bool useOldConvexityDefectMethod = false, Fubi::FingerCountImageData* debugData = 0x0);

	// Releases an image previously created by the FubiImageProcessing methods
	static void releaseImage(void* image);

private:
	// Static class so no public constructor
	FubiImageProcessing();

	// Draw the color image of the sensor, returns true if succesful
	static bool drawColorImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, bool swapBandR = false);
	// Draw the ir image of the sensor, returns true if succesful
	static bool drawIRImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth);
	// Draws the depth histogram with optional tracking info to the given image buffer, returns true if succesful
	static bool drawDepthImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, Fubi::DepthImageModification::Modification depthModifications, unsigned int renderOptions);
	// Adds tracking info to a image
	static void drawTrackingInfo(FubiISensor* sensor, unsigned char* outputImage, int width, int height, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, unsigned int renderOptions);
	// overlay the last finger count image onto the output image
	static void drawFingerCountImage(unsigned int userID, bool leftHand, unsigned char* outputImage, int width, int height, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth);

	// Draw a single limp of a player in a image buffer
	static void drawLimb(unsigned int player, Fubi::SkeletonJoint::Joint eJoint1, Fubi::SkeletonJoint::Joint eJoint2, unsigned char* outputImage, int width, int height,
		Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, unsigned int renderOptions);
	// Draw the label for a body measurement
	static void drawBodyMeasurement(unsigned int player, Fubi::SkeletonJoint::Joint eJoint1, Fubi::SkeletonJoint::Joint eJoint2, Fubi::BodyMeasurement::Measurement bodyMeasure, unsigned char* outputImage, int width, int height, 
	Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, unsigned int renderOptions);

	// Helper function for setting the image roi around the user joint (and thresholding the image) returns true if user and joint were found
	static bool setROIToUserJoint(void* pImage, unsigned int userId, Fubi::SkeletonJoint::Joint jointOfInterest, int applyThreshold = 0);
	// Helper function for applying a two sided threshold (also for 16 bit images)
	static void applyThreshold(void* pImage, unsigned int min, unsigned int max, unsigned int replaceValue = 0);

	// Counts the number fingers in a depth image that should have set its roi to the region of one hand (and threshold applied preferrably)
	// The processing steps will be visualized into the rgbImage if given
	static int fingerCount(void * pDepthImage, void* pRgbaImage = 0x0, bool useContourDefectMode = false);

	// Buffer for calculating the depth histogram
	static float m_depthHist[Fubi::MaxDepth];
	static unsigned short m_lastMaxDepth;

	// The different colors for each user id
	static const float m_colors[Fubi::MaxUsers+1][3];

	/*static FubiImageProcessing* s_instance;*/
};
