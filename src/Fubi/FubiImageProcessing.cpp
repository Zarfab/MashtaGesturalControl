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


// Defines for enabling/disabling OpenNI and OpenCV dependencies
#include "FubiConfig.h"

// OpenCV includes
#if defined USE_OPENCV || defined USE_OPENCV_22
#	define USE_OPENCV
#	include "opencv/cv.h"
#	include "opencv/highgui.h"
#	include <opencv/cxcore.h>
#	ifdef USE_OPENCV_22
#		ifdef _DEBUG
#			pragma comment(lib, "opencv_highgui220d.lib")
#			pragma comment(lib, "opencv_core220d.lib")
#			pragma comment(lib, "opencv_imgproc220d.lib")
#		else
#			pragma comment(lib, "opencv_highgui220.lib")
#			pragma comment(lib, "opencv_core220.lib")
#			pragma comment(lib, "opencv_imgproc220.lib")
#		endif
#	else
#		ifdef _DEBUG
#			pragma comment(lib, "opencv_highgui243d.lib")
#			pragma comment(lib, "opencv_core243d.lib")
#			pragma comment(lib, "opencv_imgproc243d.lib")
#		else
#			pragma comment(lib, "opencv_highgui243.lib")
#			pragma comment(lib, "opencv_core243.lib")
#			pragma comment(lib, "opencv_imgproc243.lib")
#		endif
#	endif
using namespace cv;
#endif

#include "FubiImageProcessing.h"

#include "Fubi.h"
#include "FubiUser.h"

#include <queue>
#include <sstream>

using namespace Fubi;
using namespace std;

const float FubiImageProcessing::m_colors[MaxUsers+1][3] =
{
	{1.f,1.f,1.f}, // Background
	{1.f,1.f,0}, // User 1
	{0,1.f,1.f}, // User 2
	{1.f,0,1.f}, // ...
	{1.f,0,0},
	{0,1.f,0},
	{0,0,1.f},
	{1.f,.5f,0},
	{.5f,0,1.f},
	{0,1.f,.5f},
	{1.f,0,0.5f},
	{0,.5f,1.f},
	{.5f,1.f,0},
	{1.f,.5f,1.f},
	{1.f,1.f,.5f},
	{.5f,1.f,1.f}
};

float FubiImageProcessing::m_depthHist[Fubi::MaxDepth];
unsigned short FubiImageProcessing::m_lastMaxDepth = Fubi::MaxDepth;
double lastTick, currTick, fps;
int tickIndex = 0;

FubiImageProcessing::FubiImageProcessing()
{
}

void FubiImageProcessing::getColorForUserID(unsigned int id, float& r, float& g, float& b)
{
	unsigned int nColorID = id % MaxUsers;
	b = m_colors[nColorID][0];
	g = m_colors[nColorID][1];
	r = m_colors[nColorID][2];
}


bool FubiImageProcessing::getImage(FubiISensor* sensor, unsigned char* outputImage, ImageType::Type type, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth, 
	unsigned int renderOptions /*= (RenderOptions::Shapes | RenderOptions::Skeletons | RenderOptions::UserCaptions)*/,
	DepthImageModification::Modification depthModifications /*= DepthImageModification::UseHistogram*/,
    unsigned int userId /*= 0*/, Fubi::SkeletonJoint::Joint jointOfInterest /*= Fubi::SkeletonJoint::NUM_JOINTS*/,
    FubiUserGesture current_gestures/*=FubiUserGesture()*/)
{	
	bool succes = false;
	int applyThreshold = 0;

	int width = 0, height = 0;

	// First render image
	if (type == ImageType::Color)
	{
		bool swapRAndB = (renderOptions & RenderOptions::SwapRAndB) != 0;
		succes = drawColorImage(sensor, outputImage, numChannels, depth, swapRAndB);
		Fubi::getRgbResolution(width, height);
		
		// Color image has by default the channel order RGB
		// As the tracking info's default is BGR we have to switch the swapRAndB option for the rest of the rendering
		if (swapRAndB)
			renderOptions &= ~RenderOptions::SwapRAndB;
		else
			renderOptions |= RenderOptions::SwapRAndB;
	}
	else if (type == ImageType::Depth)
	{
		succes = drawDepthImage(sensor, outputImage, numChannels, depth, depthModifications, renderOptions);
		Fubi::getDepthResolution(width, height);
		if ((userId != 0) && depthModifications != DepthImageModification::UseHistogram && depthModifications != DepthImageModification::ConvertToRGB)
		{
			if (jointOfInterest == SkeletonJoint::NUM_JOINTS || jointOfInterest == SkeletonJoint::TORSO)
				applyThreshold = 400;
			else if (jointOfInterest == SkeletonJoint::HEAD)
				applyThreshold = 200;
			else
				applyThreshold = 75;
		}
	}
	else if (type == ImageType::IR)
	{
		succes = drawIRImage(sensor, outputImage, numChannels, depth);
		Fubi::getIRResolution(width, height);
	}
	else if (type == ImageType::Blank)
	{
		succes = true;
		Fubi::getDepthResolution(width, height);
		if (width <= 0 || height <= 0)
		{
			// "Fake" standard resolution
			width = 640;
			height = 480;
		}
	}

	if (width <= 0 || height <= 0)
		succes = false; // No valid image created
//adding fps display
	if(succes)
	{
#ifdef USE_OPENCV
		if(tickIndex == 0)
		{
			currTick = Fubi::getCurrentTime();
			fps = 30.0/(currTick-lastTick);
			lastTick = currTick;
		}

		std::string s = "fps:";
		std::ostringstream os;
		os << setprecision(3) << fps;


        if(userId == 0){
            for(FubiUserGesture::iterator current_gesture = current_gestures.begin(); current_gesture != current_gestures.end(); current_gesture++){
                if(current_gesture->second !="")
                    os << " user id " << current_gesture->first << " gesture '" << current_gesture->second << "'";
            }
        }
        else{
            FubiUserGesture::iterator current_gesture = current_gestures.find(userId);
            if(current_gesture !=current_gestures.end()){
                if(current_gesture->second !="")
                    os << " user id " << current_gesture->first << " gesture '" << current_gesture->second << "'";
            }
        }

        s += os.str();
		
		IplImage* image = cvCreateImageHeader(cvSize(width, height), (depth == ImageDepth::D8) ? IPL_DEPTH_8U : IPL_DEPTH_16U, numChannels);
		image->imageData = (char*) outputImage;
		
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_DUPLEX, 0.5, 0.6, 0, 1);
		cvPutText(image, s.c_str(), cvPoint(8, 12), &font, cvScalar(255, 255, 255, 255));
		cvReleaseImageHeader(&image);

		tickIndex = (tickIndex+1) % 30;
#endif
	}
//	

	// "Crops" image if user is set
	if (succes && userId != 0)
	{
#ifdef USE_OPENCV
		IplImage* image = cvCreateImageHeader(cvSize(width, height), (depth == ImageDepth::D8) ? IPL_DEPTH_8U : IPL_DEPTH_16U, numChannels);
		image->imageData = (char*) outputImage;
		succes = setROIToUserJoint(image, userId, jointOfInterest, applyThreshold);
		if (succes)
		{
			// Get the set roi
			cv::Rect roiRect(image->roi->xOffset, image->roi->yOffset, image->roi->width, image->roi->height);
			// And reset it
			cvResetImageROI(image);
			// Put the image in a mat
			cv::Mat inverseFill(image);
			// Create a single-channel mask the same size as the image filled with 1s
			cv::Mat inverseMask(inverseFill.size(), CV_8UC1, cv::Scalar(1));
			// Specify the ROI in the mask
			cv::Mat inverseMaskROI = inverseMask(roiRect);
			//Fill the mask's ROI with 0s
			inverseMaskROI = cv::Scalar(0);
			//Set the image to 0 in places where the mask is 1
			inverseFill.setTo(cv::Scalar(0), inverseMask); // cv::Scalar(0) -> other channels should also be set to 0
		}
		cvReleaseImageHeader(&image);
#endif
	}

	// Add tracking info
	if (succes && (renderOptions != RenderOptions::None))
	{
		drawTrackingInfo(sensor, outputImage, width, height, numChannels, depth, renderOptions);
	}

	return succes;
}

void FubiImageProcessing::drawFingerCountImage(unsigned int userID, bool leftHand, unsigned char* outputImage, int width, int height, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth)
{
#ifdef USE_OPENCV
	FubiUser* user = Fubi::getUser(userID);
	if (user)
	{
		// Check scaling
		Fubi::Vec3f ImageToDepthScale(1.0f, 1.0f, 1.0f);
		int depthWidth = 0, depthHeight = 0;
		getDepthResolution(depthWidth, depthHeight);
		if (depthWidth > 0 && depthHeight > 0)
		{
			ImageToDepthScale.x = (float)width/(float)depthWidth;
			ImageToDepthScale.y = (float)height/(float)depthHeight;
		}	

		// Copy the finger count images to the wanted place
		const FingerCountImageData* fCountD = user->getFingerCountImageData(leftHand);
		if (Fubi::getCurrentTime() - fCountD->timeStamp < 0.33f && fCountD->image)
		{
			const FingerCountImageData* fCIData = fCountD;
			IplImage* fCImage = (IplImage*)fCIData->image;
			
			CvSize scaledSize = cvSize(int(ImageToDepthScale.x*fCImage->width + 0.5f), int(ImageToDepthScale.y*fCImage->height + 0.5f));
			CvPoint scaledPoint = cvPoint(int(ImageToDepthScale.x*fCIData->posX + 0.5f), int(ImageToDepthScale.y*fCIData->posY + 0.5f));

			double maxValue = 255.0;

			// Convert the finger count image to an image with correct depth and size
			IplImage* fImage = cvCreateImage(scaledSize, depth, 4);
			if (depth == ImageDepth::D16)
			{
				IplImage* d16Image = cvCreateImage(cvSize(fCImage->width, fCImage->height), IPL_DEPTH_16U, 4);
				cvConvertScale(fCImage, d16Image, (double)Math::MaxUShort16 / 255.0);
				maxValue = (double)Math::MaxUShort16;
				cvResize(d16Image, fImage);
				cvReleaseImage(&d16Image);
			}
			else
				cvResize(fCImage, fImage);

			// Now copy the finger count image into the correct place with respect to the channels
			if (depth == ImageDepth::D16)
			{
				const unsigned short* pLineStart = (unsigned short*)fImage->imageData;
				const unsigned short* pfImage = (unsigned short*)fImage->imageData;
				unsigned short* pDestLineStart = (unsigned short*)outputImage;
				unsigned short* pDestImage = (unsigned short*)outputImage;
				// Now add the shapes to the image
				for (int j = 0; j < scaledSize.height; j++)
				{
					pLineStart = ((unsigned short*)fImage->imageData) + j*scaledSize.width*4;
					pDestLineStart = ((unsigned short*)outputImage) + (j+scaledPoint.y)*width*numChannels;
					for(int i = 0; i < scaledSize.width; i++)
					{
						pfImage = pLineStart + i*4;
						pDestImage = pDestLineStart + (i+scaledPoint.x)*numChannels;

						if (pfImage[3] > 0)
						{
							double alpha = pfImage[3] / maxValue;
							double dAlpha = 1.0 - alpha;
							if (numChannels == 1)
							{
								unsigned short greyValue = (unsigned short)(0.114*pfImage[0] + 0.587*pfImage[1] + 0.299*pfImage[2]);
								pDestImage[0] = (unsigned short)(alpha*greyValue + dAlpha*pDestImage[0] + 0.5);
							}
							else
							{
								pDestImage[0] = (unsigned short)(alpha*pfImage[0] + dAlpha*pDestImage[0] + 0.5);
								pDestImage[1] = (unsigned short)(alpha*pfImage[1] + dAlpha*pDestImage[1] + 0.5);
								pDestImage[2] = (unsigned short)(alpha*pfImage[2] + dAlpha*pDestImage[2] + 0.5);
								
								if (numChannels == 4)
								{
									pDestImage[3] = std::max(pDestImage[3], pfImage[3]);
								}
							}
						}
					}
				}
			}
			else
			{
				const unsigned char* pLineStart = (unsigned char*)fImage->imageData;
				const unsigned char* pfImage = (unsigned char*)fImage->imageData;
				unsigned char* pDestLineStart = outputImage;
				unsigned char* pDestImage = outputImage;
				// Now add the shapes to the image
				for (int j = 0; j < scaledSize.height; j++)
				{
					pLineStart = (unsigned char*)fImage->imageData + j*scaledSize.width*4;
					pDestLineStart = outputImage + (j+scaledPoint.y)*width*numChannels;
					for(int i = 0; i < scaledSize.width; i++)
					{
						pfImage = pLineStart + i*4;
						pDestImage = pDestLineStart + (i+scaledPoint.x)*numChannels;

						if (pfImage[3] > 0)
						{
							double alpha = pfImage[3] / maxValue;
							double dAlpha = 1.0 - alpha;
							if (numChannels == 1)
							{
								unsigned char greyValue = (unsigned char)(0.114*pfImage[0] + 0.587*pfImage[1] + 0.299*pfImage[2]);
								pDestImage[0] = (unsigned char)(alpha*greyValue + dAlpha*pDestImage[0] + 0.5);
							}
							else
							{
								pDestImage[0] = (unsigned char)(alpha*pfImage[0] + dAlpha*pDestImage[0] + 0.5);
								pDestImage[1] = (unsigned char)(alpha*pfImage[1] + dAlpha*pDestImage[1] + 0.5);
								pDestImage[2] = (unsigned char)(alpha*pfImage[2] + dAlpha*pDestImage[2] + 0.5);
								
								if (numChannels == 4)
								{
									pDestImage[3] = std::max(pDestImage[3], pfImage[3]);
								}
							}
						}
					}
				}
			}

			// Release temporary image
			cvReleaseImage(&fImage);			
		}
	}
#else
	static double lastWarning = -99;
	if (Fubi::currentTime() - lastWarning > 10)
	{
		Fubi_logWrn("Sorry, can't draw finger count image without USE_OPENCV defined in the FubiConfig.h.\n");
		lastWarning = Fubi::currentTime();
	}
#endif
}

void FubiImageProcessing::drawTrackingInfo(FubiISensor* sensor, unsigned char* outputImage, int width, int height, ImageNumChannels::Channel numChannels, ImageDepth::Depth depth, unsigned int renderOptions)
{
	FubiUser** users;
	unsigned short numUsers = Fubi::getCurrentUsers(&users);

	if (numUsers > 0)
	{
		// Render the user shapes
		if (renderOptions & RenderOptions::Shapes)
		{
			if (sensor)
			{
				// Get user labels
				const unsigned short* pImageStart = sensor->getUserLabelData();
				if (pImageStart)
				{
					// Check for tracking/calibration per id
					bool trackedIDs[Fubi::MaxUsers];
					memset(trackedIDs, 0, sizeof(bool)*(Fubi::MaxUsers));
					for (unsigned short i = 0; i < numUsers; i++)
					{
						trackedIDs[users[i]->m_id] = users[i]->m_isTracked;
					}

					// Check scaling
					Fubi::Vec3f ImageToDepthScale(1.0f, 1.0f, 1.0f);

					int depthWidth = 0, depthHeight = 0;
					getDepthResolution(depthWidth, depthHeight);
					if (depthWidth > 0 && depthHeight > 0)
					{
						ImageToDepthScale.x = (float)depthWidth / (float)width;
						ImageToDepthScale.y = (float)depthHeight / (float)height;
					}

					const unsigned short* pLineStart = pImageStart;
					const unsigned short* pLabels = pImageStart;
					unsigned int nColorID = 0;
					if (depth == ImageDepth::D16)
					{
						unsigned short* pDestImage = (unsigned short*) outputImage;
						// Now add the shapes to the image
						for (int j = 0; j < height; j++)
						{
							pLineStart = pImageStart + int(j*ImageToDepthScale.y + 0.5f)*depthWidth;
							for(int i = 0; i < width; i++)
							{
								pLabels = pLineStart + int(i*ImageToDepthScale.x + 0.5f);
								if (*pLabels != 0 )
								{														
									// Add user shapes (tracked users highlighted)
									nColorID = (*pLabels) % (MaxUsers+1);

									if (renderOptions & RenderOptions::SwapRAndB)
										pDestImage[0] = (unsigned short)(pDestImage[0]*m_colors[nColorID][2]);
									else
										pDestImage[0] = (unsigned short)(pDestImage[0]*m_colors[nColorID][0]);

									if ( numChannels > 1)
									{
										pDestImage[1] = (unsigned short)(pDestImage[1]*m_colors[nColorID][1]);
										if (renderOptions & RenderOptions::SwapRAndB)
											pDestImage[2] = (unsigned short)(pDestImage[2]*m_colors[nColorID][0]);
										else
											pDestImage[2] = (unsigned short)(pDestImage[2]*m_colors[nColorID][2]);
										if (numChannels == 4)
										{
											if (trackedIDs[(*pLabels)] || (*pLabels) == 0)
												pDestImage[3] = Math::MaxUShort16;
											else
												pDestImage[3] = Math::MaxUShort16 / 2;
										}
									}
								}
								pDestImage+=numChannels;
							}
						}
					}
					else
					{
						unsigned char* pDestImage = outputImage;
						// Now add the shapes to the image
						for (int j = 0; j < height; j++)
						{
							pLineStart = pImageStart + int(j*ImageToDepthScale.y + 0.5f)*depthWidth;
							for(int i = 0; i < width; i++)
							{
								pLabels = pLineStart + int(i*ImageToDepthScale.x + 0.5f);
								if (*pLabels != 0 )
								{							
									// Add user shapes (tracked users highlighted)
									nColorID = (*pLabels) % (MaxUsers+1);
									if (renderOptions & RenderOptions::SwapRAndB)
										pDestImage[0] = (unsigned char)(pDestImage[0]*m_colors[nColorID][2]);
									else
										pDestImage[0] = (unsigned char)(pDestImage[0]*m_colors[nColorID][0]);
									if ( numChannels > 1)
									{
										pDestImage[1] = (unsigned char)(pDestImage[1]*m_colors[nColorID][1]);
										if (renderOptions & RenderOptions::SwapRAndB)
											pDestImage[2] = (unsigned char)(pDestImage[2]*m_colors[nColorID][0]);
										else
											pDestImage[2] = (unsigned char)(pDestImage[2]*m_colors[nColorID][2]);
										if (numChannels == 4)
										{
											if (trackedIDs[(*pLabels)] || (*pLabels) == 0)
												pDestImage[3] = 255;
											else
												pDestImage[3] = 128;
										}
									}
								}
								pDestImage+=numChannels;
							}
						}
					}	
				}
			}
		}

#ifdef USE_OPENCV
		// Additional render options require OpenCV

		for (unsigned short i = 0; i < numUsers; i++)
		{
			if (users[i]->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_confidence > 0
				&& users[i]->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position.z > 100.0f)
			{
				// First render finger shapes if wanted
				if (renderOptions & RenderOptions::FingerShapes)
				{
					// For left
					drawFingerCountImage(users[i]->m_id, true, outputImage, width, height, numChannels, depth);
					// and right hand
					drawFingerCountImage(users[i]->m_id, false, outputImage, width, height, numChannels, depth);
				}

				if (users[i]->m_isTracked)
				{
					if (renderOptions
					& (RenderOptions::Skeletons | RenderOptions::GlobalOrientCaptions | RenderOptions::LocalOrientCaptions
					| RenderOptions::GlobalPosCaptions | RenderOptions::LocalPosCaptions))
					{
						// Draw the user's skeleton
						drawLimb(users[i]->m_id, SkeletonJoint::NECK, SkeletonJoint::HEAD, outputImage, width, height, numChannels, depth, renderOptions);

						drawLimb(users[i]->m_id, SkeletonJoint::LEFT_SHOULDER, SkeletonJoint::NECK, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::LEFT_SHOULDER, SkeletonJoint::LEFT_ELBOW, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::LEFT_ELBOW, SkeletonJoint::LEFT_HAND, outputImage, width, height, numChannels, depth, renderOptions);

						drawLimb(users[i]->m_id, SkeletonJoint::NECK, SkeletonJoint::RIGHT_SHOULDER, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::RIGHT_SHOULDER, SkeletonJoint::RIGHT_ELBOW, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::RIGHT_ELBOW, SkeletonJoint::RIGHT_HAND, outputImage, width, height, numChannels, depth, renderOptions);

						drawLimb(users[i]->m_id, SkeletonJoint::TORSO, SkeletonJoint::LEFT_SHOULDER, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::RIGHT_SHOULDER, SkeletonJoint::TORSO, outputImage, width, height, numChannels, depth, renderOptions);

						drawLimb(users[i]->m_id, SkeletonJoint::TORSO, SkeletonJoint::LEFT_HIP, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::LEFT_HIP, SkeletonJoint::LEFT_KNEE, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::LEFT_KNEE, SkeletonJoint::LEFT_FOOT, outputImage, width, height, numChannels, depth, renderOptions);

						drawLimb(users[i]->m_id, SkeletonJoint::TORSO, SkeletonJoint::RIGHT_HIP, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::RIGHT_HIP, SkeletonJoint::RIGHT_KNEE, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::RIGHT_KNEE, SkeletonJoint::RIGHT_FOOT, outputImage, width, height, numChannels, depth, renderOptions);

						// TODO: wrists and ankles?

						// Never draw joint captions for this one as we already have...
						/*drawLimb(users[i]->m_id, SkeletonJoint::WAIST, SkeletonJoint::RIGHT_HIP, outputImage, width, height, numChannels, depth,
							(renderOptions & ~(RenderOptions::GlobalOrientCaptions | RenderOptions::LocalOrientCaptions
							| RenderOptions::GlobalPosCaptions | RenderOptions::LocalPosCaptions)));*/
						drawLimb(users[i]->m_id, SkeletonJoint::LEFT_HIP, SkeletonJoint::RIGHT_HIP, outputImage, width, height, numChannels, depth,
							(renderOptions & ~(RenderOptions::GlobalOrientCaptions | RenderOptions::LocalOrientCaptions
							| RenderOptions::GlobalPosCaptions | RenderOptions::LocalPosCaptions)));
					}

					if (renderOptions & RenderOptions::DetailedFaceShapes)
					{
						// Create image header according
						IplImage* image;
						float maxValue;
						if (depth == ImageDepth::D16)
						{
							image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_16U, numChannels);
							maxValue = Math::MaxUShort16;
						}
						else
						{
							image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, numChannels);
							maxValue = 255;
						}
						// Set image data pointer
						image->imageData = (char*) outputImage;

						// Convert to projective (screen coordinates)
						Fubi::Vec3f depthToImageScale(1.0f, 1.0f, 1.0f);

						int depthWidth = 0, depthHeight = 0;
						getDepthResolution(depthWidth, depthHeight);
						if (depthWidth > 0 && depthHeight > 0)
						{
							depthToImageScale.x = (float)width / (float)depthWidth;
							depthToImageScale.y = (float)height / (float)depthHeight;
						}

						int num = sensor->getFacePoints(users[i]->m_id, 0x0, true);
						if (num > 0)
						{
							Fubi::Vec3f* points  = new Fubi::Vec3f[num];
							Fubi::Vec3f* triangles  = new Fubi::Vec3f[num];
							sensor->getFacePoints(users[i]->m_id, points, false, triangles);

							/*
							// Code for projected 2d points
							for (int ipt = 0; ipt < 8; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[(ipt+1)%8];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 8; ipt < 16; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[(ipt-8+1)%8+8];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 16; ipt < 26; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[(ipt-16+1)%10+16];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 26; ipt < 36; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[(ipt-26+1)%10+26];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 36; ipt < 47; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[ipt+1];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 48; ipt < 60; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[(ipt-48+1)%12+48];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 60; ipt < 68; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[(ipt-60+1)%8+60];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}
							for (int ipt = 68; ipt < 86; ++ipt)
							{
								Fubi::Vec3f& ptStart = points[ipt];
								Fubi::Vec3f& ptEnd = points[ipt+1];
								cvLine(image, cvPoint((int)(ptStart.x +0.5f), (int)(ptStart.y +0.5f)), cvPoint((int)(ptEnd.x +0.5f), (int)(ptEnd.y +0.5f)), cvScalar(maxValue, maxValue, maxValue, maxValue));
							}*/

							float r, g, b;
							getColorForUserID(users[i]->m_id, r, g, b);
							for (int i = 0; i < num; ++i)
							{
								Fubi::Vec3f pos1 = Fubi::realWorldToProjective(points[(int)triangles[i].x]);
								pos1.x *= depthToImageScale.x;
								pos1.y *= depthToImageScale.y;
								Fubi::Vec3f pos2 = Fubi::realWorldToProjective(points[(int)triangles[i].y]);
								pos2.x *= depthToImageScale.x;
								pos2.y *= depthToImageScale.y;
								Fubi::Vec3f pos3 = Fubi::realWorldToProjective(points[(int)triangles[i].z]);
								pos3.x *= depthToImageScale.x;
								pos3.y *= depthToImageScale.y;
								cvLine(image, cvPoint((int)pos1.x, (int)pos1.y), cvPoint((int)pos2.x, (int)pos2.y), cvScalar(maxValue*(1.0f-b), maxValue*(1.0f-g), maxValue*(1.0f-r), maxValue));
								cvLine(image, cvPoint((int)pos2.x, (int)pos2.y), cvPoint((int)pos3.x, (int)pos3.y), cvScalar(maxValue*(1.0f-b), maxValue*(1.0f-g), maxValue*(1.0f-r), maxValue));
								cvLine(image, cvPoint((int)pos1.x, (int)pos1.y), cvPoint((int)pos3.x, (int)pos3.y), cvScalar(maxValue*(1.0f-b), maxValue*(1.0f-g), maxValue*(1.0f-r), maxValue));
							}
							delete[] points;
							delete[] triangles;
						}
					}
					else if (renderOptions & RenderOptions::Skeletons)
					{
						drawLimb(users[i]->m_id, SkeletonJoint::HEAD, SkeletonJoint::FACE_NOSE, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::FACE_CHIN, SkeletonJoint::FACE_RIGHT_EAR, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::FACE_RIGHT_EAR, SkeletonJoint::FACE_FOREHEAD, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::FACE_FOREHEAD, SkeletonJoint::FACE_LEFT_EAR, outputImage, width, height, numChannels, depth, renderOptions);
						drawLimb(users[i]->m_id, SkeletonJoint::FACE_LEFT_EAR, SkeletonJoint::FACE_CHIN, outputImage, width, height, numChannels, depth, renderOptions);
					}
				}
				if (renderOptions & RenderOptions::UserCaptions)
				{
					// Create user labels
					stringstream ss;
					ss.setf(ios::fixed,ios::floatfield); 
					ss.precision(0);

					Fubi::Vec3f pos = users[i]->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position;

					if (users[i]->m_isTracked)
					{
						// Tracking
						ss << "User"  << users[i]->m_id << "@(" << pos.x << "," << pos.y << "," << pos.z << ") Tracking";
					}
					else
					{
						// Not yet tracked = Calibrating
						ss << "User"  << users[i]->m_id << "@(" << pos.x << "," << pos.y << "," << pos.z << ") Calibrating";
					}

					string label = ss.str();

					// print text
					float r, g, b;
					getColorForUserID(users[i]->m_id, r, g, b);
					if (renderOptions & RenderOptions::SwapRAndB)
						swap(r, b);
					double maxValue = (depth == ImageDepth::D16) ? Math::MaxUShort16 : 255;
					int offset = int(3.5f * label.length());

					Fubi::Vec3f posProjective = Fubi::realWorldToProjective(pos);

					Fubi::Vec3f depthToImageScale(1.0f, 1.0f, 1.0f);
					int depthWidth = 0, depthHeight = 0;
					getDepthResolution(depthWidth, depthHeight);
					if (depthWidth > 0 && depthHeight > 0)
					{
						depthToImageScale.x = (float)width / (float)depthWidth;
						depthToImageScale.y = (float)height / (float)depthHeight;
					}

					pos.x *= depthToImageScale.x;
					pos.y *= depthToImageScale.y;

					// Now print it (opposite color)
					double hScale=0.5*depthToImageScale.x;
					double vScale=0.55*depthToImageScale.y;
					int    thickness=(int)depthToImageScale.y;
					// Create image header according
					IplImage* image;
					if (depth == ImageDepth::D16)
					{
						image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_16U, numChannels);
					}
					else
					{
						image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, numChannels);
					}

					image->imageData = (char*) outputImage;

					CvFont font;
					cvInitFont(&font, CV_FONT_HERSHEY_DUPLEX, hScale, vScale, 0, thickness);
					cvPutText (image, label.c_str(), cvPoint((int)posProjective.x-offset, (int)posProjective.y-5), &font, cvScalar((1-b)*maxValue, (1-g)*maxValue, (1-r)*maxValue, maxValue));
					cvReleaseImageHeader(&image);
				}

				if (renderOptions & RenderOptions::BodyMeasurements)
				{
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::RIGHT_FOOT, SkeletonJoint::HEAD, BodyMeasurement::BODY_HEIGHT,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::WAIST, SkeletonJoint::NECK, BodyMeasurement::TORSO_HEIGHT,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::RIGHT_SHOULDER, SkeletonJoint::LEFT_SHOULDER, BodyMeasurement::SHOULDER_WIDTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::RIGHT_HIP, SkeletonJoint::LEFT_HIP, BodyMeasurement::HIP_WIDTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::RIGHT_SHOULDER, SkeletonJoint::RIGHT_HAND, BodyMeasurement::ARM_LENGTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::LEFT_SHOULDER, SkeletonJoint::LEFT_ELBOW, BodyMeasurement::UPPER_ARM_LENGTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::LEFT_ELBOW, SkeletonJoint::LEFT_HAND, BodyMeasurement::LOWER_ARM_LENGTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::RIGHT_FOOT, SkeletonJoint::RIGHT_HIP, BodyMeasurement::LEG_LENGTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::LEFT_HIP, SkeletonJoint::LEFT_KNEE, BodyMeasurement::UPPER_LEG_LENGTH,
						outputImage, width, height, numChannels, depth, renderOptions);
					drawBodyMeasurement(users[i]->m_id, SkeletonJoint::LEFT_KNEE, SkeletonJoint::LEFT_FOOT, BodyMeasurement::LOWER_LEG_LENGTH,
						outputImage, width, height, numChannels, depth, renderOptions);
				}
			}
		}
#endif
	}
}


bool FubiImageProcessing::drawDepthImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, Fubi::DepthImageModification::Modification depthModifications, unsigned int renderOptions)
{
	if (sensor)
	{
		// Get options for the resolution
		Fubi::StreamOptions options = sensor->getDepthOptions();

		// Get depth data	
		const unsigned short* pDepth = sensor->getDepthData();

		if (options.isValid() && pDepth != 0)
		{
			if (depthModifications == DepthImageModification::Raw
				&& (renderOptions == RenderOptions::None || (renderOptions & RenderOptions::Background) != 0)
				&& numChannels == ImageNumChannels::C1 && depth == ImageDepth::D16)
			{
				// Catch the special case that the source and target formats are the same
				memcpy(outputImage, pDepth, options.m_width*options.m_height*sizeof(unsigned short));
				return true;
			}

			unsigned short nValue = 0, nValue1 = 0, nValue2 = 0;
			unsigned short maxDepth = MaxDepth;

			if (depthModifications == DepthImageModification::UseHistogram)
			{
				// Calculate depth histogram
				unsigned short nIndex = 0;
				float nNumberOfPoints = 0;		
				memset(m_depthHist, 0, m_lastMaxDepth*sizeof(float));
				maxDepth = 0;
				for (unsigned short nY=0; nY<options.m_height; nY++)
				{
					for (unsigned short nX=0; nX<options.m_width; nX++)
					{
						nValue = *pDepth;
						if (nValue != 0)
						{
							m_depthHist[nValue]++;
							nNumberOfPoints++;
							if (nValue > maxDepth)
								maxDepth = nValue;
						}
						pDepth++;
					}
				}
				m_lastMaxDepth = maxDepth;

				if (nNumberOfPoints > 0 && maxDepth > 0)
				{
					// Already add first value
					m_depthHist[1] += m_depthHist[0];
					// Calculate the rest
					for (nIndex=2; nIndex<maxDepth; nIndex++)
					{
						m_depthHist[nIndex] += m_depthHist[nIndex-1];
						m_depthHist[nIndex-1] = 1.0f - (m_depthHist[nIndex-1] / nNumberOfPoints);
					}
					// And divide the last one
					m_depthHist[maxDepth] = 1.0f - (m_depthHist[maxDepth] / nNumberOfPoints);
				}
			}
			else if (depthModifications == DepthImageModification::StretchValueRange
				|| depthModifications == DepthImageModification::ConvertToRGB)
			{
				maxDepth = 0;
				// Only calculate maxDepth
				for (unsigned short nY=0; nY<options.m_height; nY++)
				{
					for (unsigned short nX=0; nX<options.m_width; nX++)
					{
						nValue = *pDepth;
						if (nValue > maxDepth)
							maxDepth = nValue;
						pDepth++;
					}
				}
			}

			const unsigned short* pLabels = sensor->getUserLabelData();
			// Get user labels
			if (pLabels == 0x0)
			{
				// No users so never render shapes, but the background
				renderOptions &= ~RenderOptions::Shapes;
				renderOptions |= RenderOptions::Background;
			}

			pDepth = sensor->getDepthData();
			unsigned char* p8DestImage;
			unsigned short* p16DestImage;
			int max = Math::MaxUShort16;
			float stretchFac = (float)Math::MaxUShort16 / (float)maxDepth;
			int lb;
			if (depth == ImageDepth::D8)
			{
				max = 255;
				stretchFac = 255.0f / (float)maxDepth;
			}
			for (int j = 0; j < options.m_height; j++)
			{
				if (depth == ImageDepth::D16)
					p16DestImage = (unsigned short*)outputImage + j*options.m_width*numChannels;
				else
					p8DestImage = outputImage + j*options.m_width*numChannels;
				for(int i = 0; i < options.m_width; i++)
				{
					if (renderOptions == RenderOptions::None || (renderOptions & RenderOptions::Background) != 0 || ((renderOptions & RenderOptions::Shapes) != 0 && *pLabels != 0))
					{
						nValue = *pDepth;
						if (depthModifications == DepthImageModification::UseHistogram)
						{
							nValue = (unsigned short)((float)max * m_depthHist[nValue]);
							nValue2 = nValue1 = nValue;
						}
						else if (depthModifications == DepthImageModification::ConvertToRGB)
						{
							if (nValue == 0)
							{
								nValue2 = nValue1 = 0;
							}
							else
							{
								if (depth == ImageDepth::D16)
								{
									lb = (nValue & 511) << 7;
								}
								else
								{
									lb = (nValue & 511) >> 1;
								}

								switch (nValue >> 9)
								{
								case 0:
									nValue = max;
									nValue1 = max-lb;
									nValue2 = max-lb;
									break;
								case 1:
									nValue = max;
									nValue1 = lb;
									nValue2 = 0;
									break;
								case 2:
									nValue = max-lb;
									nValue1 = max;
									nValue2 = 0;
									break;
								case 3:
									nValue = 0;
									nValue1 = max;
									nValue2 = lb;
									break;
								case 4:
									nValue = 0;
									nValue1 = max-lb;
									nValue2 = max;
									break;
								case 5:
									nValue = 0;
									nValue1 = 0;
									nValue2 = max-lb;
									break;
								default:
									nValue2 = nValue1 = nValue = (unsigned short)(stretchFac*(nValue-3071));
									break;
								}
							}
						}
						else if (depthModifications == DepthImageModification::StretchValueRange)
						{
							nValue2 = nValue1 = nValue = (unsigned short)(stretchFac * (float)nValue);
						}
						else
						{
							if (depth == ImageDepth::D8)
								nValue = (unsigned short)(255.0f * (float)nValue / MaxDepth);
							nValue2 = nValue1 = nValue;
						}

						if (renderOptions & RenderOptions::SwapRAndB)
						{
							swap(nValue, nValue2);
						}

						if (depth == ImageDepth::D16)
						{
							p16DestImage[0] = nValue;
							if (numChannels > 1)
							{
								p16DestImage[1] = nValue1;
								p16DestImage[2] = nValue2;
								if (numChannels == 4)
									p16DestImage[3] = Math::MaxUShort16;
							}
						}
						else
						{
							p8DestImage[0] = (unsigned char)(nValue);
							if (numChannels > 1)
							{
								p8DestImage[1] = (unsigned char)(nValue1);
								p8DestImage[2] = (unsigned char)(nValue2);
								if (numChannels == 4)
									p8DestImage[3] = 255;
							}
						}
					}
					else
					{
						// No value to put here, so make it black
						if (depth == ImageDepth::D16)
						{
							p16DestImage[0] = 0;
							if (numChannels > 1)
							{
								p16DestImage[1] = 0;
								p16DestImage[2] = 0;
								if (numChannels == 4)
									p16DestImage[3] = 0;
							}
						}
						else
						{
							p8DestImage[0] = 0;
							if (numChannels > 1)
							{
								p8DestImage[1] = 0;
								p8DestImage[2] = 0;
								if (numChannels == 4)
									p8DestImage[3] = 0;
							}
						}
					}


					pDepth++;
					if (renderOptions & RenderOptions::Shapes)
						// Only needed to be incremented if shape rendering is active
						pLabels++;
					if (depth == ImageDepth::D16)
						p16DestImage+=numChannels;
					else
						p8DestImage+=numChannels;

				}
			}
			return true;
		}
	}
	return false;
}

bool FubiImageProcessing::saveImage(FubiISensor* sensor, const char* fileName, int jpegQuality, 
	Fubi::ImageType::Type type, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth,
	unsigned int renderOptions /*= (Fubi::RenderOptions::Shapes | Fubi::RenderOptions::Skeletons | Fubi::RenderOptions::UserCaptions)*/,
	Fubi::DepthImageModification::Modification depthModifications /*= Fubi::DepthImageModification::UseHistogram*/,
	unsigned int userId /*= 0*/, SkeletonJoint::Joint jointOfInterest /*= SkeletonJoint::NUM_JOINTS*/)
{
	bool succes = false;
#ifdef USE_OPENCV
	if (sensor)
	{
		IplImage* image = 0;
		int applyThreshold = 0;
		Fubi::StreamOptions options;

		if (type == ImageType::Color)
		{
			options = sensor->getRgbOptions();
			if (options.isValid())
			{
				image = cvCreateImage(cvSize(options.m_width, options.m_height), depth, numChannels);
				// Color image has by default the channel order RGB
				// As OpenCV wants BGR as the default, we use the SwapRAndB option in the opposite way
				succes = drawColorImage(sensor, (unsigned char*)(image->imageData), numChannels, depth, (renderOptions & RenderOptions::SwapRAndB) == 0);
			}
		}
		else if (type == ImageType::IR)
		{
			options = sensor->getIROptions();
			if (options.isValid())
			{
				image = cvCreateImage(cvSize(options.m_width, options.m_height), depth, numChannels);
				succes = drawIRImage(sensor, (unsigned char*)(image->imageData), numChannels, depth);
			}
		}
		else
		{
			options = sensor->getDepthOptions();
			if (options.isValid())
			{
				image = cvCreateImage(cvSize(options.m_width, options.m_height), depth, numChannels);
				if ((userId != 0) && depthModifications != DepthImageModification::UseHistogram && depthModifications != DepthImageModification::ConvertToRGB)
				{
					if (jointOfInterest == SkeletonJoint::NUM_JOINTS || jointOfInterest == SkeletonJoint::TORSO)
						applyThreshold = 400;
					else if (jointOfInterest == SkeletonJoint::HEAD)
						applyThreshold = 200;
					else
						applyThreshold = 75;
				}
				succes = drawDepthImage(sensor, (unsigned char*)(image->imageData), numChannels, depth, depthModifications, renderOptions);
			}
		}

		if (succes && userId != 0)
		{
			succes = succes && setROIToUserJoint(image, userId, jointOfInterest, applyThreshold);
		}

		if (succes && (renderOptions != RenderOptions::None))
		{

			drawTrackingInfo(sensor, (unsigned char*)(image->imageData), options.m_width, options.m_height, numChannels, depth, renderOptions);
		}

		if (succes)
		{
			// Save to file with given quality
			int quality[3] = 
			{
				CV_IMWRITE_JPEG_QUALITY,
				jpegQuality,
				0,
			};
			succes = cvSaveImage(fileName, image, quality) != 0;
		}
		cvReleaseImage(&image);
	}
#else
	static double lastWarning = -99;
	if (Fubi::currentTime() - lastWarning > 10)
	{
		Fubi_logWrn("Sorry, can't save picture without USE_OPENCV defined in the FubiConfig.h.\n");
		lastWarning = Fubi::currentTime();
	}
#endif

	return succes;
}


bool FubiImageProcessing::drawColorImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, bool swapBandR /*= false*/)
{
	// Catch unsupported cases
	if (depth != ImageDepth::D8)
	{
		return false;
	}

	if (sensor)
	{
		// Get options for the resolution
		Fubi::StreamOptions options = sensor->getRgbOptions();
		// Get image
		const unsigned char* data = sensor->getRgbData();
		if (options.isValid() && data != 0x0)
		{
			if (numChannels == ImageNumChannels::C3)
			{
				// Directly copy image data
				memcpy(outputImage, data, options.m_width*options.m_height*sizeof(unsigned char)*3);

#ifdef USE_OPENCV
				if (swapBandR)
				{
					IplImage* image = cvCreateImageHeader(cvSize(options.m_width, options.m_height), IPL_DEPTH_8U, 3);
					image->imageData = (char*) outputImage;
					cvCvtColor(image, image, CV_BGR2RGB);
					cvReleaseImageHeader(&image);
				}
			}
			else if (numChannels == ImageNumChannels::C1)
			{
				// Convert to grayscale
				IplImage* image = cvCreateImageHeader(cvSize(options.m_width, options.m_height), IPL_DEPTH_8U, 3);
				image->imageData = (char*) data;
				IplImage* greyImage = cvCreateImageHeader(cvSize(options.m_width, options.m_height), IPL_DEPTH_8U, 1);
				greyImage->imageData = (char*) outputImage;

				cvCvtColor(image, greyImage, CV_BGR2GRAY);

				cvReleaseImageHeader(&image);
				cvReleaseImageHeader(&greyImage);
			}
			else if (numChannels == ImageNumChannels::C4)
			{
				// Add alpha channel
				IplImage* image = cvCreateImageHeader(cvSize(options.m_width, options.m_height), IPL_DEPTH_8U, 3);
				image->imageData = (char*) data;
				IplImage* rgbaImage = cvCreateImageHeader(cvSize(options.m_width, options.m_height), IPL_DEPTH_8U, 4);
				rgbaImage->imageData = (char*) outputImage;

				if (swapBandR)
				{
					cvCvtColor(image, rgbaImage, CV_BGR2RGBA);
				}
				else
				{
					cvCvtColor(image, rgbaImage, CV_BGR2BGRA);
				}

				cvReleaseImageHeader(&image);
				cvReleaseImageHeader(&rgbaImage);
			}
#else
			}

			static double lastWarning = -99;
			if (Fubi::currentTime() - lastWarning > 10)
			{
				Fubi_logWrn("Sorry, can't convert picture without USE_OPENCV defined in the FubiConfig.h.\n");
				lastWarning = Fubi::currentTime();
			}
#endif

			return true;
		}
	}
	return false;
}

bool FubiImageProcessing::drawIRImage(FubiISensor* sensor, unsigned char* outputImage, Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth)
{
	if (sensor)
	{
		// Get options for the resolution
		Fubi::StreamOptions options = sensor->getIROptions();

		// Get image		
		const unsigned short* pIr = sensor->getIrData();
		if (options.isValid() && pIr != 0x0)
		{
			unsigned short nValue = 0;
			unsigned char* p8DestImage;
			unsigned short* p16DestImage;
			for (size_t j = 0; j < (unsigned)options.m_height; j++)
			{
				if (depth == ImageDepth::D16)
					p16DestImage = (unsigned short*)outputImage + j*options.m_width*numChannels;
				else
					p8DestImage = outputImage + j*options.m_width*numChannels;
				for(size_t i = 0; i < (unsigned)options.m_width; i++)
				{
					if (*pIr != 0)
					{
						if (depth == ImageDepth::D16)
						{
							nValue = *pIr;
							p16DestImage[0] = nValue;
							if (numChannels > 1)
							{
								p16DestImage[1] = nValue;
								p16DestImage[2] = nValue;
								if (numChannels == 4)
									p16DestImage[3] = Math::MaxUShort16;
							}
						}
						else
						{
							nValue = (unsigned int)(255.0f * (float)(*pIr) / MaxIR);
							p8DestImage[0] = (unsigned char)(nValue);
							if (numChannels > 1)
							{
								p8DestImage[1] = (unsigned char)(nValue);
								p8DestImage[2] = (unsigned char)(nValue);
								if (numChannels == 4)
									p8DestImage[3] = 255;
							}
						}
					}

					pIr++;
					if (depth == ImageDepth::D16)
						p16DestImage+=numChannels;
					else
						p8DestImage+=numChannels;
				}
			}

			return true;
		}
	}
	return false;
}


void FubiImageProcessing::drawLimb(unsigned int player, Fubi::SkeletonJoint::Joint eJoint1, Fubi::SkeletonJoint::Joint eJoint2, unsigned char* outputImage, int width, int height, 
	Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, unsigned int renderOptions)
{
#ifdef USE_OPENCV
	FubiUser* user = Fubi::getUser(player);
	if (user && user->m_isTracked)
	{
		// Get positions
		SkeletonJointPosition joint1 = user->m_currentTrackingData.jointPositions[eJoint1];
		SkeletonJointPosition joint2 = user->m_currentTrackingData.jointPositions[eJoint2];

		// Check confidence
		if (joint2.m_position.z > 100.0f)
		{
			// Create image header according
			IplImage* image;
			float maxValue;
			if (depth == ImageDepth::D16)
			{
				image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_16U, numChannels);
				maxValue = Math::MaxUShort16;
			}
			else
			{
				image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, numChannels);
				maxValue = 255;
			}
			// Set image data pointer
			image->imageData = (char*) outputImage;

			// Convert to projective (screen coordinates)
			Fubi::Vec3f pos1 = Fubi::realWorldToProjective(joint1.m_position);
			Fubi::Vec3f pos2 = Fubi::realWorldToProjective(joint2.m_position);

			Fubi::Vec3f depthToImageScale(1.0f, 1.0f, 1.0f);

			int depthWidth = 0, depthHeight = 0;
			getDepthResolution(depthWidth, depthHeight);
			if (depthWidth > 0 && depthHeight > 0)
			{
				depthToImageScale.x = (float)width / (float)depthWidth;
				depthToImageScale.y = (float)height / (float)depthHeight;
			}

			pos1.x *= depthToImageScale.x;
			pos1.y *= depthToImageScale.y;
			pos2.x *= depthToImageScale.x;
			pos2.y *= depthToImageScale.y;

			// Get color
			float r, g, b;
			double a;
			if (joint2.m_confidence < 0.25f)
			{
				r = g = b = 0.5f;
				a = maxValue / 4;
			}
			else
			{
				getColorForUserID(player, r, g, b);
				if (renderOptions & RenderOptions::SwapRAndB)
					swap(r, b);
				if (joint2.m_confidence < 0.75f)
					a = maxValue / 2;
				else
					a = maxValue;
			}

			double hScale=0.45*depthToImageScale.x;
			double vScale=0.5*depthToImageScale.y;
			int    thickness=(int)depthToImageScale.y;


			if (renderOptions & RenderOptions::Skeletons)
			{
				// Draw limb
				if (pos1.z > 100.0f)
				{
					if (joint1.m_confidence > 0.25 && pos2.z > 100.0f)
					{
						cvLine(image, cvPoint((int)pos1.x, (int)pos1.y), cvPoint((int)pos2.x, (int)pos2.y), cvScalar(a*(1-b), a*(1-g), a*(1-r), a));
					}
					//cvCircle(image, cvPoint((int)pos1.x, (int)pos1.y), 10000 / (int)pos1.z, cvScalar(a*(1-b), a*(1-g), a*(1-r), a), -1);
					cvCircle(image, cvPoint((int)pos2.x, (int)pos2.y), 10000 / (int)pos2.z, cvScalar(a*(1-b), a*(1-g), a*(1-r), a), -1);
				}
			}


			if (joint2.m_confidence > 0.25f)
			{
				// Draw info for joint2
				stringstream ss;
				ss.setf(ios::fixed,ios::floatfield);
				ss.precision(0);

				if (renderOptions & RenderOptions::LocalOrientCaptions)
				{
					Fubi::Vec3f jRot = user->m_currentTrackingData.localJointOrientations[eJoint2].m_orientation.getRot();
					ss << Fubi::getJointName(eJoint2) << ":" << jRot.x << "/" << jRot.y << "/" << jRot.z;
				}
				else if (renderOptions & RenderOptions::GlobalOrientCaptions)
				{
					Fubi::Vec3f jRot = user->m_currentTrackingData.jointOrientations[eJoint2].m_orientation.getRot();
					ss << Fubi::getJointName(eJoint2) << ":" << jRot.x << "/" << jRot.y << "/" << jRot.z;
				}
				else if (renderOptions & RenderOptions::LocalPosCaptions)
				{
					const Fubi::Vec3f& jPos = user->m_currentTrackingData.localJointPositions[eJoint2].m_position;
					ss << Fubi::getJointName(eJoint2) << ":" << jPos.x << "/" << jPos.y << "/" << jPos.z;
				}
				else if (renderOptions & RenderOptions::GlobalPosCaptions)
				{
					const Fubi::Vec3f& jPos = user->m_currentTrackingData.jointPositions[eJoint2].m_position;
					ss << Fubi::getJointName(eJoint2) << ":" << jPos.x << "/" << jPos.y << "/" << jPos.z;
				}

				if (ss.str().length() > 0)
				{
					CvFont font;
					cvInitFont(&font, CV_FONT_HERSHEY_DUPLEX, hScale, vScale, 0, thickness);
					cvPutText(image, ss.str().c_str(), cvPoint((int)pos2.x + 10, (int)pos2.y + 10), &font, cvScalar((1-b)*maxValue, (1-g)*maxValue, (1-r)*maxValue, a));
				}
			}


			// Release only the header
			cvReleaseImageHeader(&image);
		}
	}
#endif
}

void FubiImageProcessing::drawBodyMeasurement(unsigned int player, Fubi::SkeletonJoint::Joint eJoint1, Fubi::SkeletonJoint::Joint eJoint2, Fubi::BodyMeasurement::Measurement bodyMeasure, unsigned char* outputImage, int width, int height, 
	Fubi::ImageNumChannels::Channel numChannels, Fubi::ImageDepth::Depth depth, unsigned int renderOptions)
{
#ifdef USE_OPENCV
	FubiUser* user = Fubi::getUser(player);
	if (user && user->m_isTracked)
	{
		// Get positions
		SkeletonJointPosition joint1 = user->m_currentTrackingData.jointPositions[eJoint1];
		SkeletonJointPosition joint2 = user->m_currentTrackingData.jointPositions[eJoint2];
		// And the measurement
		BodyMeasurementDistance bm = user->m_bodyMeasurements[bodyMeasure];

		// Check confidence
		if (joint2.m_position.z > 100.0f)
		{
			// Create image header accordingly
			IplImage* image;
			float maxValue;
			if (depth == ImageDepth::D16)
			{
				image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_16U, numChannels);
				maxValue = Math::MaxUShort16;
			}
			else
			{
				image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, numChannels);
				maxValue = 255;
			}
			// Set image data pointer
			image->imageData = (char*) outputImage;

			// Convert to projective (screen coordinates)
			Fubi::Vec3f pos1 = Fubi::realWorldToProjective(joint1.m_position);
			Fubi::Vec3f pos2 = Fubi::realWorldToProjective(joint2.m_position);

			Fubi::Vec3f depthToImageScale(1.0f, 1.0f, 1.0f);

			int depthWidth = 0, depthHeight = 0;
			getDepthResolution(depthWidth, depthHeight);
			if (depthWidth > 0 && depthHeight > 0)
			{
				depthToImageScale.x = (float)width / (float)depthWidth;
				depthToImageScale.y = (float)height / (float)depthHeight;
			}

			pos1.x *= depthToImageScale.x;
			pos1.y *= depthToImageScale.y;
			pos2.x *= depthToImageScale.x;
			pos2.y *= depthToImageScale.y;

			// Get color
			float r, g, b;
			double a;
			if (bm.m_confidence < 0.25f)
			{
				r = g = b = 0.5f;
				a = maxValue / 4;
			}
			else
			{
				getColorForUserID(player, r, g, b);
				if (renderOptions & RenderOptions::SwapRAndB)
					swap(r, b);
				if (bm.m_confidence < 0.75f)
					a = maxValue / 2;
				else
					a = maxValue;
			}

			double hScale=0.45*depthToImageScale.x;
			double vScale=0.5*depthToImageScale.y;
			int    thickness=(int)depthToImageScale.y;


			stringstream ss;				
			ss.setf(ios::fixed,ios::floatfield);
			ss.precision(0);

			Fubi::Vec3f center = pos1 + (pos2-pos1)*0.5f;
			ss << Fubi::getBodyMeasureName(bodyMeasure) << ":" << bm.m_dist;
			CvFont font;
			cvInitFont(&font, CV_FONT_HERSHEY_DUPLEX, hScale, vScale, 0, thickness);
			cvPutText(image, ss.str().c_str(), cvPoint((int)center.x, (int)center.y), &font, cvScalar((1-b)*maxValue, (1-g)*maxValue, (1-r)*maxValue, a));


			// Release only the header
			cvReleaseImageHeader(&image);
		}
	}
#endif
}


bool FubiImageProcessing::setROIToUserJoint(void* pImage, unsigned int userId, Fubi::SkeletonJoint::Joint jointOfInterest, int applyThreshold /*= 0*/)
{
	bool foundRoi = false;
#ifdef USE_OPENCV
	IplImage* image = (IplImage*)pImage;

	// Check for the user
	FubiUser* user = getUser(userId);
	if (user)
	{
		// Cut out a shape roughly around the joint of interest
		Fubi::Vec3f depthToImageScale(1.0f, 1.0f, 1.0f);
		int depthWidth = 0, depthHeight = 0;
		getDepthResolution(depthWidth, depthHeight);
		if (depthWidth > 0 && depthHeight > 0)
		{
			depthToImageScale.x = (float)image->width / (float)depthWidth;
			depthToImageScale.y = (float)image->height / (float)depthHeight;
		}

		// First get the region of interest
		int width = image->width;
		int height = image->height;
		int x = width/2, y = height/2;
		float z = 0;
		if (jointOfInterest == SkeletonJoint::NUM_JOINTS) // Cut out whole user
		{
			Fubi::Vec3f pos = user->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position;
			z = pos.z;
			pos = Fubi::realWorldToProjective(pos);
			x = int(depthToImageScale.x * pos.x);
			y = int(depthToImageScale.y * pos.y);
			// clamp a rectangle about 90 x 200 cm
			width = int(0.7 * image->width);
			height = int(1.75 * image->height);
			foundRoi = true;
		}
		else if (user->m_isTracked)	// Standard case
		{
			// Try to get the joint pos
			SkeletonJointPosition jPos = user->m_currentTrackingData.jointPositions[jointOfInterest];
			if (jPos.m_confidence > 0.5f)
			{
				Fubi::Vec3f pos = jPos.m_position;
				z = pos.z;
				pos = Fubi::realWorldToProjective(pos);
				x = int(depthToImageScale.x * pos.x);
				y = int(depthToImageScale.y * pos.y);
				// clamp a rectangle about 30 x 50 cm
				width = int(0.234 * image->width);
				height = int(0.4375 * image->height);
				foundRoi = true;
			}
		}

		if (foundRoi)
		{
			// Clamp z from 30 cm to 5 m, convert to meter, and invert it
			float zFac = 1000.0f / clamp(z, 300.0f, 5000.0f);
			// Apply z-factor
			width = int(width*zFac + 0.5f);
			height = int(height*zFac + 0.5f);
			// Set x and y from center to upper left corner and clamp it
			int upperLeftX = clamp(x-(width/2), 0, image->width-1);
			int upperLeftY = clamp(y-(height/2), 0, image->height-1);

			// Clamp size
			width = clamp(width, 1, image->width-upperLeftX);
			height = clamp(height, 1, image->height-upperLeftY);

			// Now crop the part around the joint from the image
			cvSetImageROI(image, cvRect(upperLeftX, upperLeftY, width, height));

			if (applyThreshold > 0)
			{
				// Clamp depth values according to hand depth
				int convertedZ = int(z + 0.5f);
				if (image->depth != IPL_DEPTH_16U)
					convertedZ = int((z * 255.0f / (float)Math::MaxUShort16) + 0.5f);
				FubiImageProcessing::applyThreshold((void*)image, (unsigned int)clamp(convertedZ - applyThreshold, 0, MaxDepth), (unsigned int)clamp(convertedZ + applyThreshold, 0, MaxDepth), 0);
			}
		}
	}
#else
	static double lastWarning = -99;
	if (Fubi::currentTime() - lastWarning > 10)
	{
		Fubi_logWrn("Sorry, can't process image without USE_OPENCV defined in the FubiConfig.h.\n");
		lastWarning = Fubi::currentTime();
	}
#endif

	return foundRoi;
}

void FubiImageProcessing::applyThreshold(void* pImage, unsigned int min, unsigned int max, unsigned int replaceValue /*= 0*/)
{
#ifdef USE_OPENCV
	IplImage* image = (IplImage*)pImage;
	unsigned short* data16 = (unsigned short*)(image->imageData);

	CvSize size = cvSize(image->width, image->height);
	CvSize offset = cvSize(0, 0);
	if (image->roi)
	{
		size.width = image->roi->width;
		size.height = image->roi->height;
		offset.width = image->roi->xOffset;
		offset.height = image->roi->yOffset;
	}

	if (image->depth == IPL_DEPTH_16U)
	{
		unsigned short* pData = data16;
		for (int y = offset.height; y < offset.height+size.height; ++y)
		{
			pData = data16 + (y*image->width*image->nChannels + offset.width*image->nChannels);
			for (int x = 0; x < size.width; ++x, ++pData)
			{
				*pData = clamp(*pData, (unsigned short)min, (unsigned short)max, (unsigned short)replaceValue);
			}
		}
	}
	else
	{
		char* pData = image->imageData;
		for (int y = offset.height; y < offset.height+size.height; ++y)
		{
			pData = image->imageData + y*image->width*image->nChannels;
			for (int x = 0; x < size.width; ++x, ++pData)
			{
				*pData = clamp((unsigned char)(*pData), (unsigned char)min, (unsigned char)max, (unsigned char)replaceValue);
			}
		}
	}
#else
	static double lastWarning = -99;
	if (Fubi::currentTime() - lastWarning > 10)
	{
		Fubi_logWrn("Sorry, can't apply threshold on image without USE_OPENCV defined in the FubiConfig.h.\n");
		lastWarning = Fubi::currentTime();
	}
#endif
}

int FubiImageProcessing::fingerCount(void* pDepthImage, void* pRgbaImage /*= 0x0*/, bool useContourDefectMode /*= false*/)
{
	int numFingers = -1;

#ifdef USE_OPENCV
	IplImage* depthImage = (IplImage*) pDepthImage;
	IplImage* rgbaImage = (IplImage*) pRgbaImage;

	// Get image (or roi) size  and pos
	CvRect rect = cvRect(0, 0, depthImage->width, depthImage->height);
	if (depthImage->roi)
	{
		rect.width = depthImage->roi->width;
		rect.height = depthImage->roi->height;
		rect.x = depthImage->roi->xOffset;
		rect.y = depthImage->roi->yOffset;
	}

	if (useContourDefectMode)
	{
		// Find contours
		CvMemStorage* storage = cvCreateMemStorage();
		CvSeq* contours;	

		int numContours = cvFindContours(depthImage, storage, &contours, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

		if(numContours > 0)
		{
			// Take the contour with the biggest bounding box = hand
			vector<CvRect> boxes;
			for(CvSeq* c = contours; c; c = c->h_next)
			{
				boxes.push_back(cvBoundingRect(c));
			}
			int max = 0;
			CvSeq* handContour = contours;
			CvRect boundbox = cvBoundingRect(contours);
			for (unsigned int j=0; j < boxes.size(); j++)
			{
				int size = boxes[j].width * boxes[j].height;
				if(size > max)
				{
					max = size;
					handContour = contours;
					boundbox = boxes[j];
				}
				contours = contours->h_next;
			}

			// Only take big enough pictures, it wont make any sense else
			if (max > 250)
			{
				if (rgbaImage)
				{
					// Draw the bounding box
					cvRectangle(rgbaImage, cvPoint(boundbox.x, boundbox.y), cvPoint(boundbox.x+boundbox.width, boundbox.y+boundbox.height), cvScalar(0, 255, 255, 255));
					// Draw contours in the rgba image
					cvDrawContours(rgbaImage, handContour, cvScalar(255,0,255,255), cvScalar(255,0,0,255), 1, 2);
				}

				// Get convex hull
				CvMemStorage* storage1 = cvCreateMemStorage();
				CvSeq* convexHull = cvConvexHull2(handContour, storage1, CV_CLOCKWISE, 0);

				// Calculate convexity defects
				CvMemStorage* storage2 = cvCreateMemStorage();	
				CvSeq* defects = cvConvexityDefects(handContour, convexHull, storage2);
				// Convert to array
				CvConvexityDefect* defectArray = (CvConvexityDefect*) malloc(sizeof(CvConvexityDefect)*defects->total);
				cvCvtSeqToArray(defects, defectArray, CV_WHOLE_SEQ); 
				unsigned int numSmallAngles = 0;
				unsigned int numLargeCenteredDefects = 0;
				for (int i = 0; i < defects->total; ++i)
				{
					bool defectCounted = false;

					float defectRelY = float(defectArray[i].depth_point->y - boundbox.y) / boundbox.height;
					float defectDToW = defectArray[i].depth / boundbox.width;
					float defectDToH = defectArray[i].depth / boundbox.height;

					// Filter out defects with wrong size or y position (in contour bounding box as well as in the whole image)
					int maxY = rect.height * 9 / 10; // 90 % of the whole image size
					if (defectDToH > 0.1f && defectDToW > 0.2f && defectDToH < 0.75f &&  defectRelY > 0.1f && defectRelY < 0.6f &&
						defectArray[i].start->y <= maxY && defectArray[i].depth_point->y <= maxY && defectArray[i].end->y <= maxY)
					{
						numLargeCenteredDefects++;

						// Filter out defects with too large angles (only searching for the spaces between stretched fingers)
						CvPoint vec1 = cvPoint(defectArray[i].start->x - defectArray[i].depth_point->x, defectArray[i].start->y - defectArray[i].depth_point->y);
						CvPoint vec2 = cvPoint(defectArray[i].end->x - defectArray[i].depth_point->x, defectArray[i].end->y - defectArray[i].depth_point->y);
						float angle = abs(atan2f((float)vec2.y, (float)vec2.x) - atan2f((float)vec1.y, (float)vec1.x));
						if (angle > Math::Pi)
							angle = Math::TwoPi - angle;
						float a = 0.3f;
						if (angle < 90.0f * (Math::Pi / 180.0f)) // Angle smaller than 90 degrees
						{
							defectCounted = true;
							numSmallAngles++;
							a = 1.0f;
						}

						if (rgbaImage)
						{
							// Render defect edges
							cvLine(rgbaImage, *defectArray[i].start, *defectArray[i].end, cvScalar(0, a*150, 0, a*255));
							cvLine(rgbaImage, *defectArray[i].start, *defectArray[i].depth_point, cvScalar(0, a*255, a*255, a*255));
							cvLine(rgbaImage, *defectArray[i].depth_point, *defectArray[i].end, cvScalar(a*255, a*255, 0, a*255));
							// And vertices
							cvCircle(rgbaImage, *(defectArray[i].end), 5, cvScalar(0,a*255,0, a*255), -1);
							cvCircle(rgbaImage, *(defectArray[i].start), 5, cvScalar(a*255,0,0, a*255), -1);
							cvCircle(rgbaImage, *(defectArray[i].depth_point), 5, cvScalar(a*255,a*255,0, a*255), -1);
						}
					}

					/*if (defectCounted)
					{
					Fubi_logInfo("defectDToH %.3f\n", defectDToH);
					Fubi_logInfo("defectDToW %.3f\n", defectDToW);
					Fubi_logInfo("defectRelY %.3f\n", defectRelY);
					}
					else if ((defectDToH > 0.1f && defectDToH < 0.75f) || defectDToW > 0.2f)
					{
					Fubi_logInfo("---defectDToH %.3f\n", defectDToH);
					Fubi_logInfo("---defectDToW %.3f\n", defectDToW);
					Fubi_logInfo("---defectRelY %.3f\n", defectRelY);
					Fubi_logInfo("---startY %d\n", defectArray[i].start->y);
					Fubi_logInfo("---depthY %d\n", defectArray[i].depth_point->y);
					Fubi_logInfo("---endY %d\n", defectArray[i].end->y);
					}*/
				}

				if (numSmallAngles > 0)
					numFingers = numSmallAngles + 1; // Defects with small angles = space between fingers
				else if (numLargeCenteredDefects > 0)
					numFingers = 1; // Only defects with large angles = one finger up
				else //no defects = no fingers
					numFingers = 0;

				free(defectArray);
				cvReleaseMemStorage(&storage1);
				cvReleaseMemStorage(&storage2);
			}
		}

		cvReleaseMemStorage(&storage);
	}
	else
	{
		//cvThreshold(depthImage, depthImage, 100, 255, CV_THRESH_BINARY);
		cvSmooth(depthImage, depthImage, CV_MEDIAN, 7);

		CvMemStorage* storage = cvCreateMemStorage();
		CvSeq* hand_contour;
		cvFindContours(depthImage, storage, &hand_contour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
		if (hand_contour)
		{
			numFingers = 0;
			CvSeq* handd = hand_contour;
			float bigArea = 0, area = 0;
			while(hand_contour->h_next)
			{
				area = (float)abs(cvContourArea(hand_contour, CV_WHOLE_SEQ));
				if(area > bigArea)
				{
					bigArea = area; 
					handd = hand_contour;
				}
				hand_contour =  hand_contour->h_next;
			}

			cvDrawContours(depthImage, hand_contour, cvScalar(95), cvScalar(95), 0, -1);
			cvInRangeS(depthImage, cvScalar(90), cvScalar(96), depthImage);

			cvReleaseMemStorage(&storage);

			Mat grey = cvarrToMat(depthImage);
			medianBlur(grey, grey, 7);
			erode(grey, grey, Mat(), Point(-1, -1), 1); 

			Mat e = getStructuringElement(CV_SHAPE_ELLIPSE, Size(5,5), Point(3,3));

			Mat morphImage, subImage;
			morphologyEx(grey, morphImage, CV_MOP_OPEN, e, Point(-1, -1), 4);
			subtract(grey, morphImage, subImage);	
			threshold(subImage, subImage, 100, 255, CV_THRESH_BINARY);

			Point2f centre;
			Moments moment = moments(grey, true);
			centre.x = float(moment.m10 /moment.m00);
			centre.y = float(moment.m01 /moment.m00);

			vector<vector<Point> > contours;
			vector<Vec4i> hierarchy;
			findContours( subImage, contours, hierarchy, CV_RETR_LIST, CHAIN_APPROX_SIMPLE);
			subImage.release();
			e.release();
			morphImage.release();
			grey.release();

			if (rgbaImage)
			{
				Mat rgba = cvarrToMat(rgbaImage); 
				line(rgba, cvPoint(int(centre.x - 20.5f), int(centre.y + 0.5f)), cvPoint(int(centre.x + 20.5f), int(centre.y + 0.5f)), Scalar(208, 224, 64, 255), 2, 8);//light blue
				line(rgba, cvPoint(int(centre.x + 0.5f), int(centre.y - 20.5f)), cvPoint(int(centre.x + 0.5f), int(centre.y + 20.5f)), Scalar(208, 224, 64, 255), 2, 8);//light blue
			}

			if (hierarchy.size() > 0)
			{
				Point2f u,v;
				Mat p;
				float yD1, yD2, xD1, xD2, distance1, distance2, finger_area;
				for(int idx = 0, i = 0; idx >= 0 && hierarchy.size() > (unsigned)idx; idx = hierarchy[idx][0], i++)
				{
					if(numFingers < 5 )
					{	
						finger_area = (float)contourArea(Mat(contours[i]));
						if(finger_area > 40 )
						{
							numFingers++;
							if (rgbaImage)
							{
								Mat rgba = cvarrToMat(rgbaImage);
								drawContours(rgba, contours, idx, Scalar(238, 178, 0, 255), CV_FILLED, 8, hierarchy );
								Mat(contours[i]).convertTo(p, CV_32F);
								RotatedRect box = fitEllipse(p);
								ellipse(rgba, box.center, box.size * 0.5f, box.angle, 0, 360, Scalar(64, 64, 255, 255), 2);//melon 
								Point2f vtx[4];
								box.points(vtx);

								u.x = (vtx[1].x  + vtx[2].x)/2;
								u.y = (vtx[1].y  + vtx[2].y)/2;

								v.x = (vtx[0].x  + vtx[3].x)/2;
								v.y = (vtx[0].y  + vtx[3].y)/2;

								xD1 = u.x - centre.x;
								yD1 = u.y - centre.y;			
								distance1 = sqrt(pow(xD1, 2) + pow(yD1, 2));

								xD2 = v.x - centre.x;
								yD2 = v.y - centre.y;			
								distance2 = sqrt(pow(xD2, 2) + pow(yD2, 2));

								if(distance1 > distance2 )
								{
									circle(rgba, u, 6, Scalar(180, 110, 255, 255), -1);//pink
								}else 
								{
									circle(rgba, v, 6, Scalar(180, 110, 255, 255), -1);//pink
								}
							}
						}
					}
					else
						break;
				}
				p.release();
			}
		}
	}

#else
	static double lastWarning = -99;
	if (Fubi::currentTime() - lastWarning > 10)
	{
		Fubi_logWrn("Sorry, can't apply finger detection on image without USE_OPENCV defined in the FubiConfig.h.\n");
		lastWarning = Fubi::currentTime();
	}
#endif

	return numFingers;
}

int FubiImageProcessing::applyFingerCount(FubiISensor* sensor, unsigned int userID, bool leftHand /*= false*/, bool useOldConvexityDefectMethod /*= false*/, FingerCountImageData* debugData /*= 0x0*/)
{
	int numFingers = -1;

#ifdef USE_OPENCV
	if (sensor)
	{
		Fubi::StreamOptions options = sensor->getDepthOptions();

		// Retrieve the depth image
		IplImage* image = cvCreateImage(cvSize(options.m_width, options.m_height), IPL_DEPTH_16U, 1);
		drawDepthImage(sensor, (unsigned char*)image->imageData, ImageNumChannels::C1, ImageDepth::D16, DepthImageModification::Raw, RenderOptions::None);

		// Set Region of interest to the observed hand and apply a depth threshold
		if (setROIToUserJoint(image, userID, leftHand ? SkeletonJoint::LEFT_HAND : SkeletonJoint::RIGHT_HAND, 75))
		{
			// Convert the image back to 8 bit (easier to handle in the rest)		
			IplImage* depthImage = cvCreateImage(cvSize(image->roi->width, image->roi->height), IPL_DEPTH_8U, 1);
			// And by they way convert it to a binary image
			cvConvertScale(image, depthImage, 255.0 /*/ (double)Math::MaxUShort16*/);

			IplImage* rgbaImage = 0x0;
			if (debugData)
			{
				// Convert the image to rgb
				rgbaImage = cvCreateImage(cvSize(image->roi->width, image->roi->height), IPL_DEPTH_8U, 4);
				//cvCvtColor(depthImage, rgbaImage, CV_GRAY2BGRA);
				cvZero(rgbaImage);
				debugData->image = (void*)rgbaImage;
				debugData->posX = image->roi->xOffset;
				debugData->posY = image->roi->yOffset;
			}

			// Try to detect the number of fingers and render the contours in the image
			numFingers = fingerCount(depthImage, rgbaImage, useOldConvexityDefectMethod);

			if (debugData)
			{
				debugData->timeStamp = Fubi::getCurrentTime();
				debugData->fingerCount = numFingers;
			}

			cvReleaseImage(&depthImage);
		}

		cvReleaseImage(&image);
	}
#else
	static double lastWarning = -99;
	if (Fubi::currentTime() - lastWarning > 10)
	{
		Fubi_logWrn("Can't apply finger detection on image without USE_OPENCV defined in the FubiConfig.h.\n");
		lastWarning = Fubi::currentTime();
	}
#endif
	return numFingers;
}

void FubiImageProcessing::releaseImage(void* image)
{
#ifdef USE_OPENCV
	if (image)
	{
		IplImage* img = (IplImage*) image;
		cvReleaseImage(&img);
	}
#endif
	// Nothing to release in the other case
}
