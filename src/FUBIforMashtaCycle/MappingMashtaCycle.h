#pragma once
#include <string>
#include <vector>
#include <map>
#include "FubiUser.h"

typedef struct MessageToSend
{
	std::string text;
	std::vector<float> values;
};


typedef enum MashtaSoundControl
{
	LOOP,
	STOP,
	REVERB_FREEZE,
	VOLUME,
	SPEED,
	REVERB_MIX,
	PAN,
	POSITION,
	NB_SOUND_CONTROL
};


class MappingMashtaCycle
{
public:
	MappingMashtaCycle(void);
	MappingMashtaCycle(float sw, float sd, float sdo);
	~MappingMashtaCycle(void);
	MessageToSend getOSCMessage(FubiUser* user, std::string comboName);
	void initMapping();
	
private:
	int boundValue(float *value, float up, float low);
	MessageToSend loopMessage(FubiUser* user);
	MessageToSend stopMessage(FubiUser* user);	
	MessageToSend reverbFreezeMessage(FubiUser* user);	
	MessageToSend volumeMessage(FubiUser* user);	
	MessageToSend speedMessage(FubiUser* user);	
	MessageToSend reverbMixMessage(FubiUser* user);
	MessageToSend panMessage(FubiUser* user);	
	MessageToSend positionMessage(FubiUser* user);

	bool reverbFreeze[16];
	float sceneWidth, sceneDepth, sceneDepthOffset;

	std::map<std::string, MashtaSoundControl> mapping;
};

