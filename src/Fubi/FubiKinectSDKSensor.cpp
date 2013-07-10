// ****************************************************************************************
//
// Fubi Kinect SDK sensor
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#include "FubiKinectSDKSensor.h"

#ifdef USE_KINECT_SDK

#pragma comment(lib, "FaceTrackLib.lib")
#pragma comment(lib, "Kinect10.lib")


#include "FubiUser.h"
#include "Fubi.h"


using namespace Fubi;
using namespace std;

static NUI_SKELETON_POSITION_INDEX JointToKSDKJoint(const SkeletonJoint::Joint j)
{
	switch (j)
	{
	case SkeletonJoint::HEAD:
		return NUI_SKELETON_POSITION_HEAD;
	case SkeletonJoint::NECK:
		return NUI_SKELETON_POSITION_SHOULDER_CENTER;
	case SkeletonJoint::TORSO:
		return NUI_SKELETON_POSITION_SPINE;
	case SkeletonJoint::LEFT_SHOULDER:
		return NUI_SKELETON_POSITION_SHOULDER_LEFT;
	case SkeletonJoint::LEFT_ELBOW:
		return NUI_SKELETON_POSITION_ELBOW_LEFT;
	case SkeletonJoint::LEFT_WRIST:
		return NUI_SKELETON_POSITION_WRIST_LEFT;
	case SkeletonJoint::LEFT_HAND:
		return NUI_SKELETON_POSITION_HAND_LEFT;
	case SkeletonJoint::RIGHT_SHOULDER:
		return NUI_SKELETON_POSITION_SHOULDER_RIGHT;
	case SkeletonJoint::RIGHT_ELBOW:
		return NUI_SKELETON_POSITION_ELBOW_RIGHT;
	case SkeletonJoint::RIGHT_WRIST:
		return NUI_SKELETON_POSITION_WRIST_RIGHT;
	case SkeletonJoint::RIGHT_HAND:
		return NUI_SKELETON_POSITION_HAND_RIGHT;
	case SkeletonJoint::LEFT_HIP:
		return NUI_SKELETON_POSITION_HIP_LEFT;
	case SkeletonJoint::LEFT_KNEE:
		return NUI_SKELETON_POSITION_KNEE_LEFT;
	case SkeletonJoint::LEFT_ANKLE:
		return NUI_SKELETON_POSITION_ANKLE_LEFT;
	case SkeletonJoint::LEFT_FOOT:
		return NUI_SKELETON_POSITION_FOOT_LEFT;
	case SkeletonJoint::RIGHT_HIP:
		return NUI_SKELETON_POSITION_HIP_RIGHT;
	case SkeletonJoint::RIGHT_KNEE:
		return NUI_SKELETON_POSITION_KNEE_RIGHT;
	case SkeletonJoint::RIGHT_ANKLE:
		return NUI_SKELETON_POSITION_ANKLE_RIGHT;
	case SkeletonJoint::RIGHT_FOOT:
		return NUI_SKELETON_POSITION_FOOT_RIGHT;
	case SkeletonJoint::WAIST:
		return NUI_SKELETON_POSITION_HIP_CENTER;
	}
	return NUI_SKELETON_POSITION_COUNT;
}

static NUI_SKELETON_POSITION_INDEX JointToKSDKFallbackJoint(const SkeletonJoint::Joint j)
{
	switch (j)
	{
	case SkeletonJoint::LEFT_HAND:
		return NUI_SKELETON_POSITION_WRIST_LEFT;
	case SkeletonJoint::RIGHT_HAND:
		return NUI_SKELETON_POSITION_WRIST_RIGHT;
	case SkeletonJoint::LEFT_FOOT:
		return NUI_SKELETON_POSITION_ANKLE_LEFT;
	case SkeletonJoint::RIGHT_FOOT:
		return NUI_SKELETON_POSITION_ANKLE_RIGHT;
	}
	return NUI_SKELETON_POSITION_COUNT;
}

static int JointToKSDKFacePointIndex(const SkeletonJoint::Joint j)
{
	switch (j)
	{
	case SkeletonJoint::FACE_NOSE:
		return 7;
	case SkeletonJoint::FACE_LEFT_EAR:
		return 53;
	case SkeletonJoint::FACE_RIGHT_EAR:
		return 20;
	case SkeletonJoint::FACE_FOREHEAD:
		return 1;
	case SkeletonJoint::FACE_CHIN:
		return 10;
	}
	return 0;
}

static void NUIMatrix4ToMatrix3f(const Matrix4& nuiMat, Matrix3f& mat)
{
	mat.c[0][0] = nuiMat.M11;
	mat.c[0][1] = nuiMat.M12;
	mat.c[0][2] = nuiMat.M13;
	mat.c[1][0] = nuiMat.M21;
	mat.c[1][1] = nuiMat.M22;
	mat.c[1][2] = nuiMat.M23;
	mat.c[2][0] = nuiMat.M31;
	mat.c[2][1] = nuiMat.M32;
	mat.c[2][2] = nuiMat.M33;

	// Convert coordinate system
	Vec3f rot = mat.getRot();
	rot.y -= 180.0f ;
	rot.x *= -1.0f;
	rot.z *= -1.0f;
	mat = Matrix3f(Quaternion(degToRad(rot.x), degToRad(rot.y), degToRad(rot.z)));
}

static Fubi::Vec3f NUIVec3ToVec3f(const FT_VECTOR3D& nuiVec)
{
	return Vec3f(nuiVec.x*1000.0f, nuiVec.y*1000.0f, nuiVec.z*1000.0f);
}

static Fubi::Vec3f NUIVec2ToVec3f(const FT_VECTOR2D& nuiVec)
{
	return Vec3f(nuiVec.x, nuiVec.y, 0);
}

static Fubi::Vec3f NUITriangleIndexToVec3f(const FT_TRIANGLE& nuiTri)
{
	return Vec3f((float)nuiTri.i, (float)nuiTri.j, (float)nuiTri.k);
}

FubiKinectSDKSensor::FubiKinectSDKSensor()
{
	m_options.m_type = SensorType::KINECTSDK;
	m_hNextDepthFrameEvent = NULL;
    m_hNextVideoFrameEvent = NULL;
    m_hNextSkeletonEvent = NULL;
    m_pDepthStreamHandle = NULL;
    m_pVideoStreamHandle = NULL;
    m_hThNuiProcess=NULL;
    m_hEvNuiProcessStop=NULL;
    m_bNuiInitialized = false;
    m_framesTotal = 0;
    m_skeletonTotal = 0;
    m_videoBuffer = NULL;
    m_depthBuffer = NULL;
    m_zoomFactor = 1.0f;
    m_viewOffset.x = 0;
    m_viewOffset.y = 0;
	m_imageDataNew = false;

	for (int i=0; i < KINECT_SDK_MAX_NUM_FACES_TRACKED; ++i)
	{
		m_userContext[i].m_CountUntilFailure = 0;
		m_userContext[i].m_LastTrackSucceeded = false;
		m_userContext[i].m_SkeletonId = -1;
		m_userContext[i].m_pFaceTracker = 0x0;
		m_userContext[i].m_pFTResult = 0x0;
	}
}

HRESULT FubiKinectSDKSensor::getVideoConfiguration(FT_CAMERA_CONFIG* videoConfig)
{
    if (!videoConfig)
    {
        return E_POINTER;
    }

    UINT width = m_videoBuffer ? m_videoBuffer->GetWidth() : 0;
    UINT height =  m_videoBuffer ? m_videoBuffer->GetHeight() : 0;
    FLOAT focalLength = 0.f;

    if(width == 640 && height == 480)
    {
        focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
    }
    else if(width == 1280 && height == 960)
    {
        focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
    }

    if(focalLength == 0.f)
    {
        return E_UNEXPECTED;
    }

    videoConfig->FocalLength = focalLength;
    videoConfig->Width = width;
    videoConfig->Height = height;
    return(S_OK);
}

HRESULT FubiKinectSDKSensor::getDepthConfiguration(FT_CAMERA_CONFIG* depthConfig)
{
    if (!depthConfig)
    {
        return E_POINTER;
    }

    UINT width = m_depthBuffer ? m_depthBuffer->GetWidth() : 0;
    UINT height =  m_depthBuffer ? m_depthBuffer->GetHeight() : 0;
    FLOAT focalLength = 0.f;

    if(width == 80 && height == 60)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS / 4.f;
    }
    else if(width == 320 && height == 240)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
    }
    else if(width == 640 && height == 480)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
    }
        
    if(focalLength == 0.f)
    {
        return E_UNEXPECTED;
    }

    depthConfig->FocalLength = focalLength;
    depthConfig->Width = width;
    depthConfig->Height = height;

    return S_OK;
}



bool FubiKinectSDKSensor::initWithOptions(const Fubi::SensorOptions& options)
{
	// Init sensor streams and user tracking
	HRESULT hr = E_UNEXPECTED;
	
	NUI_IMAGE_TYPE depthType = NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX;
	NUI_IMAGE_RESOLUTION depthRes;
	switch (options.m_depthOptions.m_width)
	{
	case 80:
		depthRes = NUI_IMAGE_RESOLUTION_80x60;
		m_options.m_depthOptions.m_height = 60;
		break;
	case 320:
		depthRes = NUI_IMAGE_RESOLUTION_320x240;
		m_options.m_depthOptions.m_height = 240;
		break;
	case 1280:
		depthRes =	NUI_IMAGE_RESOLUTION_1280x960;
		m_options.m_depthOptions.m_height = 960;
	default:
		m_options.m_depthOptions.m_width = 640;
		m_options.m_depthOptions.m_height = 480;
		depthRes = NUI_IMAGE_RESOLUTION_640x480;
	}

	NUI_IMAGE_TYPE colorType;
	NUI_IMAGE_RESOLUTION colorRes;
	if (options.m_rgbOptions.isValid())
	{
		colorType = NUI_IMAGE_TYPE_COLOR;
		switch (options.m_rgbOptions.m_width)
		{
		case 80:
			colorRes = NUI_IMAGE_RESOLUTION_80x60;
			m_options.m_rgbOptions.m_height = 60;
			break;
		case 320:
			colorRes = NUI_IMAGE_RESOLUTION_320x240;
			m_options.m_rgbOptions.m_height = 240;
			break;
		case 1280:
			colorRes =	NUI_IMAGE_RESOLUTION_1280x960;
			m_options.m_rgbOptions.m_height = 960;
		default:
			m_options.m_rgbOptions.m_width = 640;
			m_options.m_rgbOptions.m_height = 480;
			colorRes = NUI_IMAGE_RESOLUTION_640x480;
		}
	}
	else
	{
		colorType = NUI_IMAGE_TYPE_COLOR_INFRARED;
		switch (options.m_irOptions.m_width)
		{
		case 80:
			colorRes = NUI_IMAGE_RESOLUTION_80x60;
			m_options.m_rgbOptions.m_height = 60;
			break;
		case 320:
			colorRes = NUI_IMAGE_RESOLUTION_320x240;
			m_options.m_rgbOptions.m_height = 240;
			break;
		case 1280:
			colorRes =	NUI_IMAGE_RESOLUTION_1280x960;
			m_options.m_rgbOptions.m_height = 960;
		default:
			m_options.m_rgbOptions.m_width = 640;
			m_options.m_rgbOptions.m_height = 480;
			colorRes = NUI_IMAGE_RESOLUTION_640x480;
		}
	}
	
	BOOL bSeatedSkeletonMode = FALSE;
	BOOL bNearMode = TRUE;
	BOOL bFallbackToDefault = TRUE;

    m_videoBuffer = FTCreateImage();
    if (!m_videoBuffer)
        return false;
	m_convertedVideoBuffer = FTCreateImage();
    if (!m_convertedVideoBuffer)
        return false;
    DWORD width = 0;
    DWORD height = 0;
    NuiImageResolutionToSize(colorRes, width, height);
    hr = m_videoBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT8_B8G8R8X8);
    if (FAILED(hr))
        return false;
	hr = m_convertedVideoBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT8_R8G8B8);
	if (FAILED(hr))
        return false;

    m_depthBuffer = FTCreateImage();
    if (!m_depthBuffer)
        return false;
	m_convertedDepthBuffer = FTCreateImage();
	m_playerPixelBuffer = FTCreateImage();
	if (!m_convertedDepthBuffer || !m_playerPixelBuffer)
		return false;
    NuiImageResolutionToSize(depthRes, width, height);
    hr = m_depthBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT16_D13P3);
    if (FAILED(hr))
        return false;
	hr = m_convertedDepthBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT16_D16);
	if (FAILED(hr))
        return false;
	hr = m_playerPixelBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT16_D16);
	if (FAILED(hr))
        return false;
    
    m_framesTotal = 0;
    m_skeletonTotal = 0;
    for (int i = 0; i < NUI_SKELETON_COUNT; ++i)
    {
		for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j)
		{
			m_skelPos[i][j] = FT_VECTOR3D(0, 0, 0);
			m_skelConf[i][j] = 0;
			m_skelRot[i][j].M11 = 1; m_skelRot[i][j].M12 = 0; m_skelRot[i][j].M13 = 0; m_skelRot[i][j].M14 = 0;
			m_skelRot[i][j].M21 = 0; m_skelRot[i][j].M22 = 1; m_skelRot[i][j].M23 = 0; m_skelRot[i][j].M24 = 0;
			m_skelRot[i][j].M31 = 0; m_skelRot[i][j].M32 = 0; m_skelRot[i][j].M33 = 1; m_skelRot[i][j].M34 = 0;
			m_skelRot[i][j].M41 = 0; m_skelRot[i][j].M42 = 0; m_skelRot[i][j].M43 = 0; m_skelRot[i][j].M44 = 1;
		}
		m_headOrient[i][0] = 0;
		m_headOrient[i][1] = 0;
		m_headOrient[i][2] = 0;
		m_headPos[i][0] = 0;
		m_headPos[i][1] = 0;
		m_headPos[i][2] = 0;
		m_headTracked[i]  = false;
		m_faceTracked[i] = false;
		m_face2DTracked[i] = false;
        m_skeletonTracked[i] = false;
    }

    m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hNextVideoFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hNextSkeletonEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    DWORD dwNuiInitDepthFlag = (depthType == NUI_IMAGE_TYPE_DEPTH)? NUI_INITIALIZE_FLAG_USES_DEPTH : NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;

    hr = NuiInitialize(dwNuiInitDepthFlag | NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR);
    if (FAILED(hr))
    {
        return false;
    }
    m_bNuiInitialized = true;

	DWORD dwSkeletonFlags = NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE;
	if (bSeatedSkeletonMode)
	{
		dwSkeletonFlags |= NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT;
	}
    hr = NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, dwSkeletonFlags );
    if (FAILED(hr))
    {
        return false;
    }

    hr = NuiImageStreamOpen(
        colorType,
        colorRes,
        0,
        2,
        m_hNextVideoFrameEvent,
        &m_pVideoStreamHandle );
    if (FAILED(hr))
    {
        return false;
    }

    hr = NuiImageStreamOpen(
        depthType,
        depthRes,
        (bNearMode)? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0,
        2,
        m_hNextDepthFrameEvent,
        &m_pDepthStreamHandle );
    if (FAILED(hr))
    {
        if(bNearMode && bFallbackToDefault)
        {
            hr = NuiImageStreamOpen(
                depthType,
                depthRes,
                0,
                2,
                m_hNextDepthFrameEvent,
                &m_pDepthStreamHandle );
        }

        if(FAILED(hr))
        {
            return false;
        }
    }

	FT_CAMERA_CONFIG videoConfig;
	getVideoConfiguration(&videoConfig);
    FT_CAMERA_CONFIG depthConfig;
	getDepthConfiguration(&depthConfig);
	for (UINT i=0; i<KINECT_SDK_MAX_NUM_FACES_TRACKED;i++)
    {
        // Try to start the face tracker.
        m_userContext[i].m_pFaceTracker = FTCreateFaceTracker();
        if (!m_userContext[i].m_pFaceTracker)
        {
            return false;
        }

        hr = m_userContext[i].m_pFaceTracker->Initialize(&videoConfig, &depthConfig, NULL, NULL); 
        if (FAILED(hr))
        {
            return false;
        }
        m_userContext[i].m_pFaceTracker->CreateFTResult(&m_userContext[i].m_pFTResult);
        if (!m_userContext[i].m_pFTResult)
        {
            return false;
        }
        m_userContext[i].m_LastTrackSucceeded = false;
		m_userContext[i].m_SkeletonId = -1;
		m_userContext[i].m_CountUntilFailure = 0;
    }

    // Start the Nui processing thread
    m_hEvNuiProcessStop=CreateEvent(NULL,TRUE,FALSE,NULL);
    m_hThNuiProcess=CreateThread(NULL,0,processThread,this,0,NULL);

	Fubi_logInfo("FubiKinectSDKSensor: succesfully initialized!\n");
	return true;
}


FubiKinectSDKSensor::~FubiKinectSDKSensor()
{
	// Stop the Nui processing thread
    if(m_hEvNuiProcessStop!=NULL)
    {
        // Signal the thread
        SetEvent(m_hEvNuiProcessStop);

        // Wait for thread to stop
        if(m_hThNuiProcess!=NULL)
        {
            WaitForSingleObject(m_hThNuiProcess,INFINITE);
            CloseHandle(m_hThNuiProcess);
            m_hThNuiProcess = NULL;
        }
        CloseHandle(m_hEvNuiProcessStop);
        m_hEvNuiProcessStop = NULL;
    }

    if (m_bNuiInitialized)
    {
        NuiShutdown();
    }
    m_bNuiInitialized = false;

    if (m_hNextSkeletonEvent && m_hNextSkeletonEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextSkeletonEvent);
        m_hNextSkeletonEvent = NULL;
    }
    if (m_hNextDepthFrameEvent && m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextDepthFrameEvent);
        m_hNextDepthFrameEvent = NULL;
    }
    if (m_hNextVideoFrameEvent && m_hNextVideoFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextVideoFrameEvent);
        m_hNextVideoFrameEvent = NULL;
    }
    if (m_videoBuffer)
    {
        m_videoBuffer->Release();
        m_videoBuffer = NULL;
    }
    if (m_depthBuffer)
    {
        m_depthBuffer->Release();
        m_depthBuffer = NULL;
    }	
	if (m_convertedVideoBuffer)
    {
        m_convertedVideoBuffer->Release();
        m_convertedVideoBuffer = NULL;
    }
    if (m_convertedDepthBuffer)
    {
        m_convertedDepthBuffer->Release();
        m_convertedDepthBuffer = NULL;
    }
	if (m_playerPixelBuffer)
    {
        m_playerPixelBuffer->Release();
        m_playerPixelBuffer = NULL;
    }

	for (UINT i=0; i<KINECT_SDK_MAX_NUM_FACES_TRACKED; i++)
	{
		if (m_userContext[i].m_pFTResult != 0)
		{
			m_userContext[i].m_pFTResult->Release();
			m_userContext[i].m_pFTResult = 0;
		}
		if (m_userContext[i].m_pFaceTracker != 0)
		{
			m_userContext[i].m_pFaceTracker->Release();
			m_userContext[i].m_pFaceTracker = 0;
		}
	}
}

DWORD WINAPI FubiKinectSDKSensor::processThread(LPVOID pParam)
{
    FubiKinectSDKSensor*  pthis=(FubiKinectSDKSensor *) pParam;
    HANDLE          hEvents[4];

    // Configure events to be listened on
    hEvents[0]=pthis->m_hEvNuiProcessStop;
    hEvents[1]=pthis->m_hNextDepthFrameEvent;
    hEvents[2]=pthis->m_hNextVideoFrameEvent;
    hEvents[3]=pthis->m_hNextSkeletonEvent;

    // Main thread loop
    while (true)
    {
        // Wait for an event to be signaled
        WaitForMultipleObjects(sizeof(hEvents)/sizeof(hEvents[0]),hEvents,FALSE,100);

        // If the stop event is set, stop looping and exit
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hEvNuiProcessStop, 0))
        {
            break;
        }

        // Process signal events
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hNextDepthFrameEvent, 0))
        {
            pthis->gotDepthAlert();
            pthis->m_framesTotal++;
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hNextVideoFrameEvent, 0))
        {
            pthis->gotVideoAlert();
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hNextSkeletonEvent, 0))
        {
            pthis->gotSkeletonAlert();
            pthis->m_skeletonTotal++;
        }
    }

    return 0;
}

void FubiKinectSDKSensor::gotVideoAlert( )
{
    const NUI_IMAGE_FRAME* pImageFrame = NULL;
    if (FAILED(NuiImageStreamGetNextFrame(m_pVideoStreamHandle, 0, &pImageFrame)))
        return;

    INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    if (SUCCEEDED(pTexture->LockRect(0, &LockedRect, NULL, 0)) && LockedRect.Pitch)
    {   // Copy video frame to face tracking
        memcpy(m_videoBuffer->GetBuffer(), PBYTE(LockedRect.pBits), min(m_videoBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));
		// Release frame
		NuiImageStreamReleaseFrame(m_pVideoStreamHandle, pImageFrame);
	
		// Convert copied data
		m_videoBuffer->CopyTo(m_convertedVideoBuffer, 0, 0, 0);

		m_imageDataNew = true;
	}
}


void FubiKinectSDKSensor::gotDepthAlert( )
{
    const NUI_IMAGE_FRAME* pImageFrame = NULL;
    if (FAILED(NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &pImageFrame)))
        return;

    INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    if (SUCCEEDED(pTexture->LockRect(0, &LockedRect, NULL, 0)) && LockedRect.Pitch)
    {   // Copy depth frame to face tracking
        memcpy(m_depthBuffer->GetBuffer(), PBYTE(LockedRect.pBits), min(m_depthBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));

		// Release frame in the meantime
		NuiImageStreamReleaseFrame(m_pDepthStreamHandle, pImageFrame);

		// Convert to non-player-id-depth
		m_depthBuffer->CopyTo(m_convertedDepthBuffer, 0, 0, 0);

		// And extract player id
		extractPlayerID(m_depthBuffer, m_playerPixelBuffer);

		m_imageDataNew = true;
	}
}

void FubiKinectSDKSensor::extractPlayerID(IFTImage* depthImage, IFTImage* pixelImage)
{
	if (depthImage->GetFormat() != FTIMAGEFORMAT_UINT16_D13P3)
		return;
	unsigned int numPixels = depthImage->GetBufferSize() / 2;
	unsigned short* depthP = (unsigned short*) depthImage->GetBuffer();
	unsigned short* playP = (unsigned short*) pixelImage->GetBuffer();
	for (unsigned int i = 0; i < numPixels; ++i)
	{
		// Extract only the 3 lowest bits as they are the player id
		*playP = (*depthP) & 0x7;
		depthP++;
		playP++;
	}
}

void FubiKinectSDKSensor::gotSkeletonAlert()
{
    NUI_SKELETON_FRAME SkeletonFrame = {0};

    HRESULT hr = NuiSkeletonGetNextFrame(0, &SkeletonFrame);
    if(FAILED(hr))
    {
        return;
    }

    for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
    {
        if( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED)
        {
            m_skeletonTracked[i] = true;
			NUI_SKELETON_BONE_ORIENTATION boneOrientations[NUI_SKELETON_POSITION_COUNT];
			NuiSkeletonCalculateBoneOrientations(&SkeletonFrame.SkeletonData[i], boneOrientations);
			for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j)
			{
				m_skelPos[i][j].x = SkeletonFrame.SkeletonData[i].SkeletonPositions[j].x;
				m_skelPos[i][j].y = SkeletonFrame.SkeletonData[i].SkeletonPositions[j].y;
				m_skelPos[i][j].z = SkeletonFrame.SkeletonData[i].SkeletonPositions[j].z;

				memcpy(&m_skelRot[i][j].M11, &boneOrientations[j].absoluteRotation.rotationMatrix.M11, sizeof(float)*16);
								
				m_skelConf[i][j] = SkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[j] / 2.0f;
			}
        }
        else
        {
            m_skeletonTracked[i] = false;
        }
    }

	m_hasNewData = true;
}

void FubiKinectSDKSensor::selectUsersToTrack(UINT nbUsers, FTHelperContext* pUserContexts)
{
	// First get the two closest users
	bool skeletonIsAvailable[NUI_SKELETON_COUNT];
    for (UINT i=0; i<NUI_SKELETON_COUNT; i++)
    {
        skeletonIsAvailable[i] = false;
    }
	std::deque<unsigned int> closestUsers = Fubi::getClosestUserIDs(nbUsers); // note Fubi ids start from 1!
	for (UINT i=0; i<closestUsers.size(); i++)
	{
		skeletonIsAvailable[closestUsers[i]-1] = true;
	}

	// Now check for already face tracked users and remove them from the available list if not lost
    for (UINT i=0; i<nbUsers; i++)
    {
        if (pUserContexts[i].m_CountUntilFailure > 0) // currently face tracked
        {
			if (m_skeletonTracked[pUserContexts[i].m_SkeletonId]) // and body is tracked
            {
                pUserContexts[i].m_CountUntilFailure = min(5, pUserContexts[i].m_CountUntilFailure+1);
            }
            else	// Body tracking failed
            {
                pUserContexts[i].m_CountUntilFailure--;
            }

			if (pUserContexts[i].m_CountUntilFailure > 0) // still trying to track, so user is not yet available
			{
				skeletonIsAvailable[pUserContexts[i].m_SkeletonId] = false;
			}
        }
	}
		
	for (UINT i=0; i<nbUsers; i++)
	{
		if (pUserContexts[i].m_CountUntilFailure == 0)	// currently no users chosen
        {
            for (UINT j=0; j<NUI_SKELETON_COUNT; j++)
            {
                if (skeletonIsAvailable[j])	// there is still one of the closest users left, so take him
                {
                    pUserContexts[i].m_SkeletonId = j;
                    pUserContexts[i].m_CountUntilFailure = 1;
					skeletonIsAvailable[j] = false;
                    break;
                }
            }
			if (pUserContexts[i].m_CountUntilFailure == 0)
			{
				if (pUserContexts[i].m_SkeletonId > 0)
				{
					m_headTracked[m_userContext[i].m_SkeletonId] = false;
					m_faceTracked[m_userContext[i].m_SkeletonId] = false;
					m_face2DTracked[m_userContext[i].m_SkeletonId] = false;
				}
				pUserContexts[i].m_SkeletonId = -1;
				m_userContext[i].m_LastTrackSucceeded = false;
			}
        }
    }
}

void FubiKinectSDKSensor::update()
{
	HRESULT hrFT = S_OK;
	// Get new stream data
	if (m_videoBuffer && m_depthBuffer && m_imageDataNew)
	{
		m_imageDataNew = false;
		FT_SENSOR_DATA sensorData(m_videoBuffer, m_depthBuffer, m_zoomFactor, &m_viewOffset);
        selectUsersToTrack(KINECT_SDK_MAX_NUM_FACES_TRACKED, m_userContext);
        for (UINT i=0; i<KINECT_SDK_MAX_NUM_FACES_TRACKED; i++)
        {
			if(m_userContext[i].m_CountUntilFailure > 0) // user has been selected
			{
				m_headTracked[m_userContext[i].m_SkeletonId] = false;
				m_faceTracked[m_userContext[i].m_SkeletonId] = false;
				m_face2DTracked[m_userContext[i].m_SkeletonId] = false;

			    FT_VECTOR3D hint[2];
				hint[0] =  m_skelPos[m_userContext[i].m_SkeletonId][NUI_SKELETON_POSITION_SHOULDER_CENTER];
				hint[1] =  m_skelPos[m_userContext[i].m_SkeletonId][NUI_SKELETON_POSITION_HEAD];

				if (m_userContext[i].m_LastTrackSucceeded)
				{
					hrFT = m_userContext[i].m_pFaceTracker->ContinueTracking(&sensorData, hint, m_userContext[i].m_pFTResult);
				}
				else
				{
					hrFT = m_userContext[i].m_pFaceTracker->StartTracking(&sensorData, NULL, hint, m_userContext[i].m_pFTResult);
				}
				m_userContext[i].m_LastTrackSucceeded = SUCCEEDED(hrFT) && SUCCEEDED(m_userContext[i].m_pFTResult->GetStatus());
				if (m_userContext[i].m_LastTrackSucceeded)
				{
					// Store head orientation
					static FLOAT scale;
					hrFT = m_userContext[i].m_pFTResult->Get3DPose(&scale, m_headOrient[m_userContext[i].m_SkeletonId], m_headPos[m_userContext[i].m_SkeletonId]);
					if (SUCCEEDED(hrFT))
					{
						m_headTracked[m_userContext[i].m_SkeletonId]  = true;

						IFTModel* ftModel;
						HRESULT hr = m_userContext[i].m_pFaceTracker->GetFaceModel(&ftModel);
						if (SUCCEEDED(hr))
						{
							FLOAT* pAUCOeffs;
							UINT pAUCOunt;
							m_userContext[i].m_pFTResult->GetAUCoefficients(&pAUCOeffs, &pAUCOunt);
					
							FLOAT* pSU = NULL;
							UINT numSU;
							BOOL suConverged;
							m_userContext[i].m_pFaceTracker->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);

							FT_VECTOR2D* pPts2D;
							UINT pts2DCount;
							m_userContext[i].m_pFTResult->Get2DShapePoints(&pPts2D, &pts2DCount);
							if (pts2DCount <= 121)
							{
								m_face2DTracked[m_userContext[i].m_SkeletonId] = true;
								for(UINT j = 0; j < pts2DCount; j++)
								{
									m_face2DPos[m_userContext[i].m_SkeletonId][j] = pPts2D[j];
								}
							}
							else
							{
								static double lastWarning = -99;
								if (Fubi::currentTime() - lastWarning > 10)
								{
									Fubi_logErr("Error in face tracking - face point count does not match!\n");
									lastWarning = Fubi::currentTime();
								}
							}

			
							UINT vertexCount = ftModel->GetVertexCount();
							if (vertexCount <= 121)
							{
								if (SUCCEEDED(ftModel->Get3DShape(pSU, numSU, pAUCOeffs, pAUCOunt, scale, m_headOrient[m_userContext[i].m_SkeletonId], m_headPos[m_userContext[i].m_SkeletonId], m_facePos[m_userContext[i].m_SkeletonId], vertexCount)))
								{
									m_faceTracked[m_userContext[i].m_SkeletonId] = true;
									FT_TRIANGLE* pTriangles;
									UINT triangleCount;
									if (SUCCEEDED(ftModel->GetTriangles(&pTriangles, &triangleCount)))
									{
										for (UINT j = 0; j < triangleCount; ++j)
										{
											m_faceTriangleIndices[m_userContext[i].m_SkeletonId][j] = pTriangles[j];
										}
									}
								}
							}
							else
							{
								static double lastWarning = -99;
								if (Fubi::currentTime() - lastWarning > 10)
								{
									Fubi_logErr("Error in face tracking - vertex count does not match!\n");
									lastWarning = Fubi::currentTime();
								}
							}
						}
					}
				}
				else
				{
					m_userContext[i].m_pFTResult->Reset();
				}
			}
        }
	}	
}


bool FubiKinectSDKSensor::hasNewTrackingData()
{
	bool newData = m_hasNewData;
	m_hasNewData = false;
	return newData;
}

bool FubiKinectSDKSensor::isTracking(unsigned int id)
{
	return m_skeletonTracked[id-1];
}

void FubiKinectSDKSensor::getSkeletonJointData(unsigned int id, Fubi::SkeletonJoint::Joint joint, Fubi::SkeletonJointPosition& position, Fubi::SkeletonJointOrientation& orientation)
{
	unsigned int userIndex = id-1;
	// Special case for torso
	if (joint == SkeletonJoint::TORSO)
	{
		// Position is middle between hip and shoulder center
		Vec3f hips;
		hips.x = m_skelPos[userIndex][NUI_SKELETON_POSITION_HIP_CENTER].x * 1000.0f;
		hips.y = m_skelPos[userIndex][NUI_SKELETON_POSITION_HIP_CENTER].y * 1000.0f;
		hips.z = m_skelPos[userIndex][NUI_SKELETON_POSITION_HIP_CENTER].z * 1000.0f;
		Vec3f shoulders;
		shoulders.x = m_skelPos[userIndex][NUI_SKELETON_POSITION_SHOULDER_CENTER].x * 1000.0f;
		shoulders.y = m_skelPos[userIndex][NUI_SKELETON_POSITION_SHOULDER_CENTER].y * 1000.0f;
		shoulders.z = m_skelPos[userIndex][NUI_SKELETON_POSITION_SHOULDER_CENTER].z * 1000.0f;
		position.m_position = hips + (shoulders-hips)*0.5f;
		position.m_confidence = minf(m_skelConf[userIndex][NUI_SKELETON_POSITION_SHOULDER_CENTER], m_skelConf[userIndex][NUI_SKELETON_POSITION_HIP_CENTER]);
		// Rotation from spine
		NUIMatrix4ToMatrix3f(m_skelRot[userIndex][NUI_SKELETON_POSITION_SPINE], orientation.m_orientation);
		orientation.m_confidence = m_skelConf[userIndex][NUI_SKELETON_POSITION_SPINE];

	}
	else if (joint >= SkeletonJoint::FACE_NOSE && joint <= SkeletonJoint::FACE_CHIN) // Special case for face
	{
		// First get head data as a basis
		getSkeletonJointData(id, SkeletonJoint::HEAD, position, orientation);
		if (m_headTracked[userIndex] && m_faceTracked[userIndex])
		{
			int fpIndex = JointToKSDKFacePointIndex(joint);
			// Now replace positions with face positions
			position.m_position.x = m_facePos[userIndex][fpIndex].x * 1000.0f;
			position.m_position.y = m_facePos[userIndex][fpIndex].y * 1000.0f;
			position.m_position.z = m_facePos[userIndex][fpIndex].z * 1000.0f;
		}
	}
	else	// Default case
	{
		NUI_SKELETON_POSITION_INDEX index = JointToKSDKJoint(joint);
		position.m_confidence = m_skelConf[userIndex][index];
		position.m_position.x = m_skelPos[userIndex][index].x * 1000.0f;
		position.m_position.y = m_skelPos[userIndex][index].y * 1000.0f;
		position.m_position.z = m_skelPos[userIndex][index].z * 1000.0f;
		orientation.m_confidence = m_skelConf[userIndex][index];
		NUIMatrix4ToMatrix3f(m_skelRot[userIndex][index], orientation.m_orientation);

		// Fallback cases for feet and hands if current confidence too low
		if (position.m_confidence < 0.4f 
			&& (index == NUI_SKELETON_POSITION_FOOT_LEFT || index == NUI_SKELETON_POSITION_FOOT_RIGHT 
				|| index == NUI_SKELETON_POSITION_HAND_LEFT || index == NUI_SKELETON_POSITION_HAND_RIGHT))
		{
			NUI_SKELETON_POSITION_INDEX index = JointToKSDKFallbackJoint(joint);
			position.m_confidence = m_skelConf[userIndex][index];
			position.m_position.x = m_skelPos[userIndex][index].x * 1000.0f;
			position.m_position.y = m_skelPos[userIndex][index].y * 1000.0f;
			position.m_position.z = m_skelPos[userIndex][index].z * 1000.0f;
			orientation.m_confidence = m_skelConf[userIndex][index];
			NUIMatrix4ToMatrix3f(m_skelRot[userIndex][index], orientation.m_orientation);
		}

		// Special case for head with face tracking activated
		if (index == NUI_SKELETON_POSITION_HEAD && m_headTracked[userIndex])
		{
			// Replace the orientation (currently no useful one) with the one from the face tracking
			orientation.m_orientation = Matrix3f(Quaternion(degToRad(m_headOrient[userIndex][0]), degToRad(m_headOrient[userIndex][1]), degToRad(m_headOrient[userIndex][2])));
			orientation.m_confidence = 1.0f; // No real confidence value, but the head is currently tracked, so this should be fine
			// Head postion by the face tracking differs significantly from that of the normal tracking
			// --> We currently don't use it...
			/*position.m_position.x = m_headPos[userIndex][0] * 1000.0f;
			position.m_position.y = m_headPos[userIndex][1] * 1000.0f;
			position.m_position.z = m_headPos[userIndex][2] * 1000.0f;
			position.m_confidence = 1.0f;*/

		}
	}
}

const unsigned short* FubiKinectSDKSensor::getDepthData()
{
	if (m_convertedDepthBuffer && m_options.m_depthOptions.isValid())
		return (unsigned short*)m_convertedDepthBuffer->GetBuffer();
	return 0x0;
}

const unsigned char* FubiKinectSDKSensor::getRgbData()
{
	if (m_convertedVideoBuffer && m_options.m_rgbOptions.isValid())
		return m_convertedVideoBuffer->GetBuffer();
	return 0x0;
}

const unsigned short* FubiKinectSDKSensor::getIrData()
{
	if (m_videoBuffer && m_options.m_irOptions.isValid())
		return (unsigned short*)m_videoBuffer->GetBuffer();
	return 0x0;
}

const unsigned short* FubiKinectSDKSensor::getUserLabelData()
{
	if (m_playerPixelBuffer && m_options.m_depthOptions.isValid() && getUserIDs(0x0) > 0)
		return (unsigned short*)m_playerPixelBuffer->GetBuffer();
	return 0x0;
}

unsigned short FubiKinectSDKSensor::getUserIDs(unsigned int* userIDs)
{
	int index = 0;
	for (int i = 0; i < NUI_SKELETON_COUNT; ++i)
	{
		if (m_skeletonTracked[i])
		{
			if (userIDs)
				userIDs[index] = i+1;
			index++;
		}
	}
	return index;
}

Fubi::Vec3f FubiKinectSDKSensor::realWorldToProjective(const Fubi::Vec3f& realWorldVec)
{
	Vec3f ret(0, 0, realWorldVec.z);

	/*if (m_userTracker.isValid())
	{
		
	}
	else*/
	{
		static const double realWorldXtoZ = tan(1.0144686707507438/2)*2;
		static const double realWorldYtoZ = tan(0.78980943449644714/2)*2;

		const double coeffX = m_options.m_depthOptions.m_width / realWorldXtoZ;
		const double coeffY = m_options.m_depthOptions.m_height / realWorldYtoZ;
		const int nHalfXres = m_options.m_depthOptions.m_width / 2;
		const int nHalfYres = m_options.m_depthOptions.m_height / 2;

		ret.x = (float)coeffX * realWorldVec.x / realWorldVec.z + nHalfXres;
		ret.y = nHalfYres - (float)coeffY * realWorldVec.y / realWorldVec.z;
	}

	return ret;
}

int FubiKinectSDKSensor::getFacePoints(unsigned int id, Fubi::Vec3f* pointArray121, bool projected2DPoints /*= false*/, Fubi::Vec3f* triangleIndexArray121 /*= 0x0*/)
{
	int num = 0;
	if (id > 0 &&  m_faceTracked[id-1])
	{
		if (!projected2DPoints)
		{
			if (pointArray121)
			{
				for (int i = 0; i < 121; ++i)
				{
					pointArray121[i] = NUIVec3ToVec3f(m_facePos[id-1][i]);
				}
			}
			num = 121;
		}

		if (projected2DPoints && m_face2DTracked[id-1])
		{
			if (pointArray121)
			{
				for (int i = 0; i < 121; ++i)
				{
					pointArray121[i] = NUIVec2ToVec3f(m_face2DPos[id-1][i]);
					pointArray121[i].z = m_facePos[id-1][i].z * 1000.0f;
				}
			}
			num = 121;
		}

		if (triangleIndexArray121)
		{
			for (int i = 0; i < 121; ++i)
			{
				triangleIndexArray121[i] =  NUITriangleIndexToVec3f(m_faceTriangleIndices[id-1][i]);
			}
		}

	}
	return num;
}
#endif