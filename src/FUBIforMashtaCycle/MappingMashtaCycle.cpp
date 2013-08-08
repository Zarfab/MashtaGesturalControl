#include "MappingMashtaCycle.h"
#include <iostream>
#include <sstream>
#include "../Fubi/Fubi.h"

using namespace Fubi;

MappingMashtaCycle::MappingMashtaCycle(void):sceneWidth(2.0), sceneDepth(1.5), sceneDepthOffset(2.0)
{
    perfMode=true;
    initPerfMapping();
    for(int i=0; i<16; i++)
        reverbFreeze[i] = false;
}

MappingMashtaCycle::MappingMashtaCycle(float sw, float sd, float sdo):sceneWidth(sw), sceneDepth(sd), sceneDepthOffset(sdo)
{
    perfMode=true;
    initPerfMapping();
    for(int i=0; i<16; i++)
        reverbFreeze[i] = false;
}


MappingMashtaCycle::~MappingMashtaCycle(void)
{
}

void MappingMashtaCycle::changeMode(bool newMode)
{
    std::string mode;

    if(newMode)
        mode = "Performative";
    else
        mode = "Installation";
    if(perfMode==newMode)
        std::cout << "Already in " << mode << " mode"<< std::endl;
    else
    {
        perfMode=newMode;
        mapping.clear();
        if(perfMode)
            initPerfMapping();
        else
            initInstallMapping();
        std::cout << "Mode changed to " << mode << std::endl;
    }


}

void MappingMashtaCycle::initPerfMapping()
{
	mapping["RightHandPushAboveShoulder"] = LOOP;
	mapping["ThrowingRightDown"] = STOP;
    mapping["Jump"] = REINIT;
	mapping["RightHandNearHead"] = REVERB_FREEZE;
	mapping["RightHandNearLeftArm"] = VOLUME;
	mapping["BothHandsInFront"] = SPEED;
	mapping["Angel"] = REVERB_MIX;
	mapping["LeftHandScanning"] = PAN;
	mapping["BothHandsDown"] = POSITION;
    mapping["ArmsParallel"] = PAUSE_ALL;
   	mapping["ArmsCrossed"] = KILL_ALL;
}

void MappingMashtaCycle::initInstallMapping()
{
    mapping["AlwaysTrue"] = POSITION;
    mapping["ThrowingRightDown"] = STOP;
    mapping["Jump"] = LOOP;
    mapping["RightHandNearHead"] = REVERB_FREEZE;
    mapping["Angel"] = VOLUME; // and REVERB_MIX
    mapping["BothHandsInFront"] = SPEED;
    mapping["LeftHandScanning"] = PAN;
}


std::vector<MessageToSend> MappingMashtaCycle::getOSCMessage(FubiUser* user, std::string comboName)
{
	std::vector<MessageToSend> vecmts;
    
	std::map<std::string, MashtaSoundControl>::iterator  it = mapping.find(comboName);
	
    
    if(it == mapping.end())
    {
        std::cout << "Combination " << comboName << " not found " << std::endl;
		return vecmts;
    }
    
	switch(it->second)
	{
		case LOOP:
			vecmts.push_back(loopMessage(user));
			break;
		case STOP:
			vecmts.push_back(stopMessage(user));
			break;
        case REINIT:
			vecmts.push_back(volumeMessage(user, 1));
            vecmts.push_back(speedMessage(user, 1));
            vecmts.push_back(panMessage(user, 0));
			break;
		case REVERB_FREEZE:
        	{
           	 	MessageToSend rfmts = reverbFreezeMessage(user);
            		if(rfmts.text != "")
                		vecmts.push_back(rfmts);
			break;
        	}
		case VOLUME:
			if(perfMode)
				vecmts.push_back(volumeMessage(user));
			else
			{
				vecmts.push_back(volumeMessage(user));
				vecmts.push_back(reverbMixMessage(user));
			break;
		case SPEED:
			vecmts.push_back(speedMessage(user));
			break;
		case REVERB_MIX:
			if(perfMode)
			{
				vecmts.push_back(reverbMixMessage(user));
				vecmts.push_back(reverbDampingMessage(user));
			}
			break;
		case PAN:
			vecmts.push_back(panMessage(user));
			break;
		case POSITION:
			vecmts.push_back(positionMessage(user));
			break;
        	case PAUSE_ALL:
			vecmts.push_back(pauseAllMessage(user));
			break;
        	case KILL_ALL:
			vecmts.push_back(killAllMessage(user));
			break;
		default:
			break;
	}
	return vecmts;
}

MessageToSend MappingMashtaCycle::getOSCPositionMessage(FubiUser* user)
{
    return positionMessage(user);
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


const float propMin = 0.67; //proportion of distance head-torso on which the reverb mix is 0
const float propMax =2.0; //proportion of distance head-torso on which the reverb mix is 1
MessageToSend MappingMashtaCycle::volumeMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_volume";
    mts.text = mes.str();
	float volume;
    
	if(perfMode)
	{
		float leftHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.y;
		float rightHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_HAND].m_position.y;
		float leftElbowY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_ELBOW].m_position.y;
    
		volume = (rightHandY-leftElbowY) / (leftHandY-leftElbowY);
	}
	else
	{
		float rightHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.y;
		float headY = user->m_currentTrackingData.jointPositions[SkeletonJoint::HEAD].m_position.y;
		float torsoY = user->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position.y;
		
		float distnorm = (rightHandY-torsoY)/(headY-torsoY);
		float a = 1/(propMax-propMin);
		
		volume = a*(distnorm-propMin);
	}
	boundValue(&volume, 1, 0);
	mts.values.push_back(volume);
    
	return mts;
}

MessageToSend MappingMashtaCycle::reverbMixMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/reverb_mix";
    mts.text = mes.str();
	float reverbMix;
    
	float rightHandX = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.x;
	float headX = user->m_currentTrackingData.jointPositions[SkeletonJoint::HEAD].m_position.x;
	float torsoX = user->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position.x;
		
	float distnorm = (rightHandX-torsoX)/(headX-torsoX);
	float a = 1/(propMax-propMin);
    
	reverbMix = a*(distnorm-propMin); //0 if distnorm < 0,67*head-torso, 1 if distnorm > 2*head-torso
	boundValue(&reverbMix, 1, 0);
	mts.values.push_back(reverbMix);
    
	return mts;
}

MessageToSend MappingMashtaCycle::reverbDampingMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/reverb_damping";
    mts.text = mes.str();
	float reverbDamp;
    if(perfMode)
	{
		float rightHandY = user->m_currentTrackingData.jointPositions[SkeletonJoint::LEFT_HAND].m_position.y;
		float headY = user->m_currentTrackingData.jointPositions[SkeletonJoint::HEAD].m_position.y;
		float torsoY = user->m_currentTrackingData.jointPositions[SkeletonJoint::TORSO].m_position.y;
		
		float distnorm = (rightHandY-torsoY)/(headY-torsoY);
		float a = 1/(propMax-propMin);
		
		reverbDamp = a*(distnorm-propMin);
	}
    
	boundValue(&reverbDamp, 1, 0);
	mts.values.push_back(reverbDamp);
    
	return mts;
}

MessageToSend MappingMashtaCycle::volumeMessage(FubiUser* user, float defaultValue)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_volume";
    mts.text = mes.str();
	float volume = defaultValue;
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

MessageToSend MappingMashtaCycle::speedMessage(FubiUser* user, float defaultValue)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_speed";
    mts.text = mes.str();
	float speed = defaultValue;
	boundValue(&speed, 5, -3);
	mts.values.push_back(speed);
    
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
	float rightShoulderX = user->m_currentTrackingData.jointPositions[SkeletonJoint::RIGHT_SHOULDER].m_position.x;
    
	pan = (leftHandX-leftShoulderX)/(rightShoulderX-leftShoulderX);
	boundValue(&pan, 1, -1);
	mts.values.push_back(pan);
    
	return mts;
}

MessageToSend MappingMashtaCycle::panMessage(FubiUser* user, float defaultValue)
{
	MessageToSend mts;
    std::ostringstream mes;
	mes << "/mediacycle/pointer/" << user->m_id << "/playback_pan";
    mts.text = mes.str();
	float pan = defaultValue;
	boundValue(&pan, 1, -1);
	mts.values.push_back(pan);
    
	return mts;
}

MessageToSend MappingMashtaCycle::positionMessage(FubiUser* user)
{
	MessageToSend mts;
    std::ostringstream mes;
    
	if(reverbFreeze[user->m_id] && perfMode)
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

MessageToSend MappingMashtaCycle::pauseAllMessage(FubiUser* user)
{
    MessageToSend mts;
    std::ostringstream mes;
    mes << "/mediacycle/pointer/" << user->m_id << "/pause_all";
	mts.text = mes.str();
	return mts;
}

MessageToSend MappingMashtaCycle::killAllMessage(FubiUser* user)
{
    MessageToSend mts;
    std::ostringstream mes;
    mes << "/mediacycle/pointer/" << user->m_id << "/kill_all";
	mts.text = mes.str();
	return mts;
}
