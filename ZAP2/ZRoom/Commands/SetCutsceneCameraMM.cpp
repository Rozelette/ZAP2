#include "SetCutsceneCameraMM.h"
#include "../ZRoom.h"
#include "../../BitConverter.h"
#include "../../StringHelper.h"

using namespace std;

SetCutsceneCameraMM::SetCutsceneCameraMM(ZRoom* nZRoom, std::vector<uint8_t> rawData, int rawDataIndex) : ZRoomCommand(nZRoom, rawData, rawDataIndex)
{
	numCutscenes = rawData[rawDataIndex + 1];
	segmentOffset = BitConverter::ToInt32BE(rawData, rawDataIndex + 4) & 0x00FFFFFF;

	_rawData = rawData;
	_rawDataIndex = rawDataIndex;

	cutscenes = vector<CutsceneCameraMMEntry*>();

	uint32_t currentPtr = segmentOffset;

	for (int i = 0; i < numCutscenes; i++)
	{
		CutsceneCameraMMEntry* entry = new CutsceneCameraMMEntry(nZRoom, _rawData, currentPtr);
		cutscenes.push_back(entry);

		currentPtr += 8;
	}

	string declaration = "";
	declaration += StringHelper::Sprintf("CutsceneCamera _%s_cutsceneCameraList_%08X[%i] = \n{\n", zRoom->GetName().c_str(), segmentOffset, cutscenes.size());

	int index = 0;
	for (CutsceneCameraMMEntry* entry : cutscenes)
	{
		declaration += StringHelper::Sprintf("\t{ %i, 0x%04X, &_%s_cutsceneCameraData_%08X }, //0x%08X\n", entry->type, entry->unk2, zRoom->GetName().c_str(), entry->segmentOffset, segmentOffset + (index * 8));

		index++;
	}

	declaration += "};\n\n";

	zRoom->declarations[segmentOffset] = new Declaration(DeclarationAlignment::Align4, DeclarationPadding::Pad16, cutscenes.size() * 8, declaration);
}

string SetCutsceneCameraMM::GenerateSourceCodePass1(string roomName, int baseAddress)
{
	string sourceOutput = "";
	char line[2048];

	sprintf(line, "%s 0x%02X, (u32)&_%s_cutsceneCameraList_%08X };", ZRoomCommand::GenerateSourceCodePass1(roomName, baseAddress).c_str(), numCutscenes, roomName.c_str(), segmentOffset);
	sourceOutput += line;


	return sourceOutput;
}

int32_t SetCutsceneCameraMM::GetRawDataSize()
{
	return ZRoomCommand::GetRawDataSize() + (cutscenes.size() * 8);
}

string SetCutsceneCameraMM::GenerateExterns()
{
	string sourceOutput = "";
	char line[2048];

	sourceOutput += StringHelper::Sprintf("extern CutsceneCamera _%s_cutsceneCameraList_%08X[%i];", zRoom->GetName().c_str(), segmentOffset, cutscenes.size());

	return sourceOutput;
}

string SetCutsceneCameraMM::GetCommandCName()
{
	return "SCmdCsCameraList";
}

RoomCommand SetCutsceneCameraMM::GetRoomCommand()
{
	return RoomCommand::SetCutsceneCameraMM;
}

CutsceneCameraMMEntry::CutsceneCameraMMEntry(ZRoom* nZRoom, std::vector<uint8_t> rawData, int rawDataIndex)
{
	type = BitConverter::ToInt16BE(rawData, rawDataIndex + 0);
	unk2 = BitConverter::ToInt16BE(rawData, rawDataIndex + 2);
	segmentOffset = BitConverter::ToInt32BE(rawData, rawDataIndex + 4) & 0x00FFFFFF;

	data = new CutsceneCameraMMData(rawData, segmentOffset);

	string declaration = "";

	declaration += StringHelper::Sprintf("CutsceneCameraData _%s_cutsceneCameraData_%08X = { %i, %i, %i, %i, %i, %i, 0x%04X, 0x%04X };\n",
		nZRoom->GetName().c_str(), segmentOffset, data->posX, data->posY, data->posZ, data->rotX, data->rotY, data->rotZ, data->fov, data->panSpeed);

	declaration += StringHelper::Sprintf("u16 CutsceneCameraDataEndMarker0x%08X = 0xFFFF;\n", segmentOffset + 0x10);

	nZRoom->declarations[segmentOffset] = new Declaration(DeclarationAlignment::None, DeclarationPadding::None, 0x12, declaration);
}

CutsceneCameraMMData::CutsceneCameraMMData(std::vector<uint8_t> rawData, int rawDataIndex)
{
	posX = BitConverter::ToInt16BE(rawData, rawDataIndex + 0);
	posY = BitConverter::ToInt16BE(rawData, rawDataIndex + 2);
	posZ = BitConverter::ToInt16BE(rawData, rawDataIndex + 4);
	rotX = BitConverter::ToInt16BE(rawData, rawDataIndex + 6);
	rotY = BitConverter::ToInt16BE(rawData, rawDataIndex + 8);
	rotZ = BitConverter::ToInt16BE(rawData, rawDataIndex + 10);
	fov = BitConverter::ToInt16BE(rawData, rawDataIndex + 12);
	panSpeed = BitConverter::ToInt16BE(rawData, rawDataIndex + 14);
}