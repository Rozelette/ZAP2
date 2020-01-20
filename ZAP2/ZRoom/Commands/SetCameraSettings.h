#pragma once

#include "../ZRoomCommand.h"

class SetCameraSettings : public ZRoomCommand
{
public:
	SetCameraSettings(ZRoom* nZRoom, std::vector<uint8_t> rawData, int rawDataIndex);

	virtual std::string GenerateSourceCodePass1(std::string roomName);
	virtual std::string GetCommandCName();
	virtual RoomCommand GetRoomCommand();

private:
	uint8_t cameraMovement;
	uint8_t mapHighlight;
};