#include "MappingMashtaCycle.h"
#include <iostream>
#include <sstream>
#include "Fubi.h"

using namespace Fubi;

MappingMashtaCycle::MappingMashtaCycle(void):sceneWidth(2.0), sceneDepth(1.5), sceneDepthOffset(2.0)
{
	initMapping();
    for(int i=0; i<16; i++)
        reverbFreeze[i] = false;
}

MappingMashtaCycle::MappingMashtaCycle(float sw, float sd, float sdo):sceneWidth(sw), sceneDepth(sd), sceneDepthOffset(sdo)
{
	initMapping();
    for(int i=0; i<16; i++)
        reverbFreeze[i] = false;
}


MappingMashtaCycle::~MappingMashtaCycle(void)
{
}

void MappingMashtaCycle::initMapping()
{
	mapping["RightHandPushAboveShoulder"] = LOOP;
	mapping["LeftHandWavingAboveHead"] = STOP;
	mapping["RightHandNearHead"] = REVERB_FREEZE;
	mapping["RightHandNearLeftArm"] = VOLUME;
	mapping["BothHandsInFront"] = SPEED;
	mapping["LeftHandOrientation3"] = REVERB_MIX;
	mapping["LeftHandScanning"] = PAN;
	mapping["BothHandsDown"] = POSITION;
}

MessageToSend MappingMashtaCycle::getOSCMessage(FubiUser* user, std::string comboName)
{
	MessageToSend mts;
	mts.text = "";

	std::map<std::string, MashtaSoundControl>::iterator  it = mapping.find(comboName);
	

    if(it == mapping.end())
    {
        std::cout << "Combination " << comboName << " not found " << std::endl;
		return mts;
    }
    
	switch(it->second)
	{
		case LOOP:
			mts = loopMessage(user);
			break;
		case STOP:
			mts = stopMessage(user);
			break;
		case REVERB_FREEZE:
			mts = reverbFreezeMessage(user);
			break;
		case VOLUME:
			mts = volumeMessage(user);
			break;
		case SPEED:
			mts = speedMessage(user);
			break;
		case REVERB_MIX:
			mts = reverbMixMessage(user);
			break;
		case PAN:
			mts = panMessage(user);
			break;
		case POSITION:
			mts = positionMessage(user);
			break;
		default:
			break;
	}
	return mts;
}

int MappingMashtaCycle::boundValue(float *value, float up, float low)
{
	if(*value > up)
	{
		*value = up;
		return 1;
	}
	if(*value < low)
	{
		*value = low;
		return -1;
	}
	else
		return 0;
}

MessageToSend MappingMashtaCycle::loopMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
    mes << "/mediacycle/pointer/" << user->m_id << "/loop";
	mts.text = mes.str();
	return mts;
}

MessageToSend MappingMashtaCycle::stopMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/mute";
    mts.text = mes.str();
	return mts;
}

MessageToSend MappingMashtaCycle::reverbFreezeMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/reverb_freeze";
    mts.text = "";
    	if(!reverbFreeze[user->m_id])
	{
		reverbFreeze[user->m_id] = true;
        mts.text = mes.str();
		mts.values.push_back(1);
	}
	return mts;
}

MessageToSend MappingMashtaCycle::volumeMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_volume";
    mts.text = mes.str();
	float volume;

	float leftHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.y;
	float rightHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND].m_position.y;
	float leftElbowY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_ELBOW].m_position.y;

	volume = (rightHandY-leftElbowY) / (leftHandY-leftElbowY);
	boundValue(&volume, 1, 0);
	mts.values.push_back(volume);

	return mts;
}

MessageToSend MappingMashtaCycle::speedMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_speed";
    mts.text = mes.str();
	float speed;

	float leftHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.y;
	float rightHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND].m_position.y;

	speed = (rightHandY-leftHandY)/100 + 1;
	boundValue(&speed, 5, -3);
	mts.values.push_back(speed);

	return mts;
}

MessageToSend MappingMashtaCycle::reverbMixMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/reverb_mix";
    mts.text = mes.str();
	float reverbMix;

	float leftHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.y;
	float leftShoulderY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER].m_position.y;

	reverbMix = 1 - (leftShoulderY-leftHandY)/400;
	boundValue(&reverbMix, 1, 0);
	mts.values.push_back(reverbMix);

	return mts;
}

MessageToSend MappingMashtaCycle::panMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_pan";
    mts.text = mes.str();
	float pan;

	float leftHandX = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.x;
	float leftShoulderX = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_SHOULDER].m_position.x;

	pan = (leftHandX-leftShoulderX)/400;
	boundValue(&pan, 1, -1);
	mts.values.push_back(pan);

	return mts;
}

MessageToSend MappingMashtaCycle::positionMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
    
	if(reverbFreeze[user->m_id])
	{
		reverbFreeze[user->m_id] = false;
        mes << "/mediacycle/pointer/" << user->m_id << "/reverb_freeze";
        mts.text = mes.str();
		mts.values.push_back(0);
	}
	else
	{	
		float xPos, yPos;

        mes << "/mediacycle/browser/" << user->m_id << "/hover/xy";
        mts.text = mes.str();
		float x = user->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position.x;
		float z = user->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position.z;

		xPos = x/(500*sceneWidth);
		yPos = 1-(z-1000*sceneDepthOffset)/(500*sceneDepth);

		boundValue(&xPos, 1, -1);
		boundValue(&yPos, 1, -1);
		mts.values.push_back(xPos);
		mts.values.push_back(yPos);
	}
	return mts;
}