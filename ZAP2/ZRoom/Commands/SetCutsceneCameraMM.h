#pragma once

#include "../ZRoomCommand.h"

class CutsceneCameraMMData
{
public:
	int16_t posX;
	int16_t posY;
	int16_t posZ;
	int16_t rotX;
	int16_t rotY;
	int16_t rotZ;
	int16_t fov;
	int16_t panSpeed;

	CutsceneCameraMMData(std::vector<uint8_t> rawData, int rawDataIndex);
};

class CutsceneCameraMMEntry
{
public:
	int16_t type;
	int16_t unk2;
	uint32_t segmentOffset;
	CutsceneCameraMMData* data;

	CutsceneCameraMMEntry(ZRoom* nZRoom, std::vector<uint8_t> rawData, int rawDataIndex);
};

class SetCutsceneCameraMM : public ZRoomCommand
{
public:
	SetCutsceneCameraMM(ZRoom* nZRoom, std::vector<uint8_t> rawData, int rawDataIndex);

	virtual std::string GenerateSourceCodePass1(std::string roomName, int baseAddress);
	virtual RoomCommand GetRoomCommand();
	virtual int32_t GetRawDataSize();
	virtual std::string GetCommandCName();
	virtual std::string GenerateExterns();

private:
	int numCutscenes;
	std::vector<CutsceneCameraMMEntry*> cutscenes;
	uint32_t segmentOffset;
	std::vector<uint8_t> _rawData;
	int _rawDataIndex;
};
