#pragma once
#include <string>
#include <vector>
#include <map>
#include "../Fubi/FubiUser.h"

struct MessageToSend
{
	std::string text;
	std::vector<float> values;
};


enum MashtaSoundControl
{
	LOOP,
	STOP,
    REINIT,
	REVERB_FREEZE,
	VOLUME,
	SPEED,
	REVERB_MIX,
	PAN,
	POSITION,
    PAUSE_ALL,
    KILL_ALL,
	NB_SOUND_CONTROL
};


class MappingMashtaCycle
{
public:
	MappingMashtaCycle(void);
	MappingMashtaCycle(float sw, float sd, float sdo);
	~MappingMashtaCycle(void);
	std::vector<MessageToSend> getOSCMessage(FubiUser* user, std::string comboName);
    MessageToSend getOSCPositionMessage(FubiUser* user);
    void changeMode(bool newMode);
    void newSceneSize(float sw, float sd, float sdo);

	
private:
    int boundValue(float *value, float up, float low);
    void initPerfMapping();
    void initInstallMapping();

    MessageToSend loopMessage(FubiUser* user);
    MessageToSend stopMessage(FubiUser* user);
    MessageToSend reverbFreezeMessage(FubiUser* user);
    MessageToSend volumeMessage(FubiUser* user);
    MessageToSend volumeMessage(FubiUser* user, float defaultValue);
    MessageToSend speedMessage(FubiUser* user);
    MessageToSend speedMessage(FubiUser* user, float defaultValue);
    MessageToSend reverbMixMessage(FubiUser* user);
    MessageToSend panMessage(FubiUser* user);
    MessageToSend panMessage(FubiUser* user, float defaultValue);
    MessageToSend positionMessage(FubiUser* user);
    MessageToSend pauseAllMessage(FubiUser* user);
    MessageToSend killAllMessage(FubiUser* user);

	bool reverbFreeze[16];
    bool perfMode; // true for performance mode, false for installation mode
	float sceneWidth, sceneDepth, sceneDepthOffset;
    
	std::map<std::string, MashtaSoundControl> mapping;
};

