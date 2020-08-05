#include <algorithm>
#include "ZNames.h"
#include "ZRoom.h"
#include "ZCutscene.h"
#include "../ZBlob.h"
#include "../File.h"
#include "../StringHelper.h"
#include "Commands/SetRoomList.h"
#include "Commands/SetEchoSettings.h"
#include "Commands/SetSoundSettings.h"
#include "Commands/SetWind.h"
#include "Commands/SetTimeSettings.h"
#include "Commands/SetSpecialObjects.h"
#include "Commands/SetSkyboxSettings.h"
#include "Commands/SetSkyboxModifier.h"
#include "Commands/SetRoomBehavior.h"
#include "Commands/SetCameraSettings.h"
#include "Commands/SetStartPositionList.h"
#include "Commands/SetActorList.h"
#include "Commands/SetTransitionActorList.h"
#include "Commands/SetEntranceList.h"
#include "Commands/SetExitList.h"
#include "Commands/SetAlternateHeaders.h"
#include "Commands/SetCollisionHeader.h"
#include "Commands/SetObjectList.h"
#include "Commands/SetMesh.h"
#include "Commands/SetLightingSettings.h"
#include "Commands/SetPathways.h"
#include "Commands/Unused09.h"
#include "Commands/SetCutscenes.h"
#include "Commands/EndMarker.h"
#include "Commands/SetCutsceneCameraMM.h"

using namespace std;
using namespace tinyxml2;

ZRoom::ZRoom(XMLElement* reader, vector<uint8_t> nRawData, int rawDataIndex, string nRelPath, ZRoom* nScene)
{
	commands = vector<ZRoomCommand*>();
	declarations = map<int32_t, Declaration*>();
	rawData = nRawData;
	name = reader->Attribute("Name");

	//printf("ZRoom: %s\n", name.c_str());

	scene = nScene;

	//GenDefinitions();

	for (XMLElement* child = reader->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		if (string(child->Name()) == "DListHint")
		{
			string comment = "";

			if (child->Attribute("Comment") != NULL)
				comment = "// " + string(child->Attribute("Comment")) + "\n";

			string addressStr = child->Attribute("Address");
			int address = strtol(StringHelper::Split(addressStr, "0x")[1].c_str(), NULL, 16);

			ZDisplayList* dList = new ZDisplayList(rawData, address, ZDisplayList::GetDListLength(rawData, address));
			declarations[address] = new Declaration(DeclarationAlignment::None, dList->GetRawDataSize(), comment + dList->GetSourceOutputCode(name));
		}
		else if (string(child->Name()) == "BlobHint")
		{
			string comment = "";

			if (child->Attribute("Comment") != NULL)
				comment = "// " + string(child->Attribute("Comment")) + "\n";

			string addressStr = child->Attribute("Address");
			int address = strtol(StringHelper::Split(addressStr, "0x")[1].c_str(), NULL, 16);

			string sizeStr = child->Attribute("Size");
			int size = strtol(StringHelper::Split(sizeStr, "0x")[1].c_str(), NULL, 16);

			char buffer[2048];
			sprintf(buffer, "%s_blob_%08X", name.c_str(), address);

			ZBlob* blob = new ZBlob(rawData, address, size, buffer);
			declarations[address] = new Declaration(DeclarationAlignment::None, blob->GetRawDataSize(), comment + blob->GetSourceOutputCode(name));
		}
		else if (string(child->Name()) == "CutsceneHint")
		{
			string comment = "";

			if (child->Attribute("Comment") != NULL)
				comment = "// " + string(child->Attribute("Comment")) + "\n";

			string addressStr = child->Attribute("Address");
			int address = strtol(StringHelper::Split(addressStr, "0x")[1].c_str(), NULL, 16);

			ZCutscene* cutscene = new ZCutscene(rawData, address, 9999);
			declarations[address] = new Declaration(DeclarationAlignment::None, DeclarationPadding::Pad16, cutscene->GetRawDataSize(), comment + cutscene->GetSourceOutputCode(name));
		}
		else if (string(child->Name()) == "AltHeaderHint")
		{
			string comment = "";

			if (child->Attribute("Comment") != NULL)
				comment = "// " + string(child->Attribute("Comment")) + "\n";

			string addressStr = child->Attribute("Address");
			int address = strtol(StringHelper::Split(addressStr, "0x")[1].c_str(), NULL, 16);

			int commandsCount = 99999999;

			if (child->FindAttribute("Count") != NULL)
			{
				string commandCountStr = child->Attribute("Count");
				commandsCount = strtol(commandCountStr.c_str(), NULL, 10);
			}

			commandSets.push_back(CommandSet(address, commandsCount));
		}
		else if (string(child->Name()) == "PathHint")
		{
			string comment = "";

			if (child->Attribute("Comment") != NULL)
				comment = "// " + string(child->Attribute("Comment")) + "\n";

			string addressStr = child->Attribute("Address");
			int address = strtol(StringHelper::Split(addressStr, "0x")[1].c_str(), NULL, 16);

			SetPathways* pathway = new SetPathways(this, rawData, address);
			pathway->InitList(address);
			pathway->GenerateSourceCodePass1(name, 0);
			pathway->GenerateSourceCodePass2(name, 0);

			delete pathway;
		}
	}

	//ParseCommands(rawDataIndex);
	commandSets.push_back(CommandSet(rawDataIndex));
	ProcessCommandSets();
}

void ZRoom::ParseCommands(std::vector<ZRoomCommand*>& commandList, CommandSet commandSet)
{
	bool shouldContinue = true;

	int currentIndex = 0;

	int rawDataIndex = commandSet.address;

	int8_t segmentNumber = rawDataIndex >> 24;

	rawDataIndex = rawDataIndex & 0x00FFFFFF;

	int32_t commandsLeft = commandSet.commandCount;

	while (shouldContinue)
	{
		RoomCommand opcode = (RoomCommand)rawData[rawDataIndex];

		ZRoomCommand* cmd = nullptr;

		if (Globals::Instance->game == ZGame::OOT)
		{
			switch (opcode)
			{
			case RoomCommand::SetStartPositionList: cmd = new SetStartPositionList(this, rawData, rawDataIndex); break; // 0x00
			case RoomCommand::SetActorList: cmd = new SetActorList(this, rawData, rawDataIndex); break; // 0x01
			case RoomCommand::SetCollisionHeader: cmd = new SetCollisionHeader(this, rawData, rawDataIndex); break; // 0x03
			case RoomCommand::SetRoomList: cmd = new SetRoomList(this, rawData, rawDataIndex); break; // 0x04
			case RoomCommand::SetWind: cmd = new SetWind(this, rawData, rawDataIndex); break; // 0x05
			case RoomCommand::SetEntranceList: cmd = new SetEntranceList(this, rawData, rawDataIndex); break; // 0x06
			case RoomCommand::SetSpecialObjects: cmd = new SetSpecialObjects(this, rawData, rawDataIndex); break; // 0x07
			case RoomCommand::SetRoomBehavior: cmd = new SetRoomBehavior(this, rawData, rawDataIndex); break; // 0x08
			case RoomCommand::Unused09: cmd = new Unused09(this, rawData, rawDataIndex); break; // 0x09
			case RoomCommand::SetMesh: cmd = new SetMesh(this, rawData, rawDataIndex); break; // 0x0A
			case RoomCommand::SetObjectList: cmd = new SetObjectList(this, rawData, rawDataIndex); break; // 0x0B
			case RoomCommand::SetPathways: cmd = new SetPathways(this, rawData, rawDataIndex); break; // 0x0D
			case RoomCommand::SetTransitionActorList: cmd = new SetTransitionActorList(this, rawData, rawDataIndex); break; // 0x0E
			case RoomCommand::SetLightingSettings: cmd = new SetLightingSettings(this, rawData, rawDataIndex); break; // 0x0F
			case RoomCommand::SetTimeSettings: cmd = new SetTimeSettings(this, rawData, rawDataIndex); break; // 0x10
			case RoomCommand::SetSkyboxSettings: cmd = new SetSkyboxSettings(this, rawData, rawDataIndex); break; // 0x11
			case RoomCommand::SetSkyboxModifier: cmd = new SetSkyboxModifier(this, rawData, rawDataIndex); break; // 0x12
			case RoomCommand::SetExitList: cmd = new SetExitList(this, rawData, rawDataIndex); break; // 0x13
			case RoomCommand::EndMarker: cmd = new EndMarker(this, rawData, rawDataIndex); break; // 0x14
			case RoomCommand::SetSoundSettings: cmd = new SetSoundSettings(this, rawData, rawDataIndex); break; // 0x15
			case RoomCommand::SetEchoSettings: cmd = new SetEchoSettings(this, rawData, rawDataIndex); break; // 0x16
			case RoomCommand::SetCutscenes: cmd = new SetCutscenes(this, rawData, rawDataIndex); break; // 0x17
			case RoomCommand::SetAlternateHeaders: cmd = new SetAlternateHeaders(this, rawData, rawDataIndex); break; // 0x18
			case RoomCommand::SetCameraSettings: cmd = new SetCameraSettings(this, rawData, rawDataIndex); break; // 0x19
			default: cmd = new ZRoomCommand(this, rawData, rawDataIndex);
			}
		}
		else
		{
			switch (opcode)
			{
			case RoomCommand::SetStartPositionList: cmd = new SetStartPositionList(this, rawData, rawDataIndex); break; // 0x00
			case RoomCommand::SetActorList: cmd = new SetActorList(this, rawData, rawDataIndex); break; // 0x01
			case RoomCommand::SetCutsceneCameraMM: cmd = new SetCutsceneCameraMM(this, rawData, rawDataIndex); break; // 0x01
			case RoomCommand::SetCollisionHeader: cmd = new SetCollisionHeader(this, rawData, rawDataIndex); break; // 0x03
			case RoomCommand::SetRoomList: cmd = new SetRoomList(this, rawData, rawDataIndex); break; // 0x04
			case RoomCommand::SetWind: cmd = new SetWind(this, rawData, rawDataIndex); break; // 0x05
			case RoomCommand::SetEntranceList: cmd = new SetEntranceList(this, rawData, rawDataIndex); break; // 0x06
			case RoomCommand::SetSpecialObjects: cmd = new SetSpecialObjects(this, rawData, rawDataIndex); break; // 0x07
			case RoomCommand::SetRoomBehavior: cmd = new SetRoomBehavior(this, rawData, rawDataIndex); break; // 0x08
			case RoomCommand::Unused09: cmd = new Unused09(this, rawData, rawDataIndex); break; // 0x09
			case RoomCommand::SetMesh: cmd = new SetMesh(this, rawData, rawDataIndex); break; // 0x0A
			case RoomCommand::SetObjectList: cmd = new SetObjectList(this, rawData, rawDataIndex); break; // 0x0B
			case RoomCommand::SetPathways: cmd = new SetPathways(this, rawData, rawDataIndex); break; // 0x0D
			case RoomCommand::SetTransitionActorList: cmd = new SetTransitionActorList(this, rawData, rawDataIndex); break; // 0x0E
			case RoomCommand::SetLightingSettings: cmd = new SetLightingSettings(this, rawData, rawDataIndex); break; // 0x0F
			case RoomCommand::SetTimeSettings: cmd = new SetTimeSettings(this, rawData, rawDataIndex); break; // 0x10
			case RoomCommand::SetSkyboxSettings: cmd = new SetSkyboxSettings(this, rawData, rawDataIndex); break; // 0x11
			case RoomCommand::SetSkyboxModifier: cmd = new SetSkyboxModifier(this, rawData, rawDataIndex); break; // 0x12
			case RoomCommand::SetExitList: cmd = new SetExitList(this, rawData, rawDataIndex); break; // 0x13
			case RoomCommand::EndMarker: cmd = new EndMarker(this, rawData, rawDataIndex); break; // 0x14
			case RoomCommand::SetSoundSettings: cmd = new SetSoundSettings(this, rawData, rawDataIndex); break; // 0x15
			case RoomCommand::SetEchoSettings: cmd = new SetEchoSettings(this, rawData, rawDataIndex); break; // 0x16
			//case RoomCommand::SetCutscenes: cmd = new SetCutscenes(this, rawData, rawDataIndex); break; // 0x17
			case RoomCommand::SetCutscenes: cmd = new EndMarker(this, rawData, rawDataIndex); break; // 0x17
			case RoomCommand::SetAlternateHeaders: cmd = new SetAlternateHeaders(this, rawData, rawDataIndex); break; // 0x18
			case RoomCommand::SetCameraSettings: cmd = new SetCameraSettings(this, rawData, rawDataIndex); break; // 0x19
			default: cmd = new ZRoomCommand(this, rawData, rawDataIndex);
			}
		}

		//printf("OP: %s\n", cmd->GetCommandCName().c_str());

		cmd->cmdIndex = currentIndex;
		cmd->cmdSet = rawDataIndex;

		commandList.push_back(cmd);

		if (opcode == RoomCommand::EndMarker)
			shouldContinue = false;

		rawDataIndex += 8;
		currentIndex++;

		commandsLeft--;

		if (commandsLeft <= 0)
			break;
	}
}

void ZRoom::ProcessCommandSets()
{
	char line[2048];

	while (commandSets.size() > 0)
	{
		std::vector<ZRoomCommand*> setCommands = std::vector<ZRoomCommand*>();

		int32_t commandSet = commandSets[0].address;
		int8_t segmentNumber = commandSet >> 24;
		ParseCommands(setCommands, commandSets[0]);
		commandSets.erase(commandSets.begin());

		//for (ZRoomCommand* cmd : setCommands)
		for (int i = 0; i < setCommands.size(); i++)
		{
			ZRoomCommand* cmd = setCommands[i];
			cmd->commandSet = commandSet & 0x00FFFFFF;

			string pass1 = cmd->GenerateSourceCodePass1(name, commandSet & 0x00FFFFFF);
			sprintf(line, "%s // 0x%04X", pass1.c_str(), cmd->cmdAddress);

			declarations[cmd->cmdAddress] = new Declaration(i == 0 ? DeclarationAlignment::Align16 : DeclarationAlignment::None, 8, line);

			sprintf(line, "extern %s _%s_set%04X_cmd%02X;\n", cmd->GetCommandCName().c_str(), name.c_str(), commandSet & 0x00FFFFFF, cmd->cmdIndex, cmd->cmdID);
			externs[cmd->cmdAddress] = line;
		}

		sourceOutput += "\n";

		for (ZRoomCommand* cmd : setCommands)
			commands.push_back(cmd);
	}

	for (ZRoomCommand* cmd : commands)
	{
		string pass2 = cmd->GenerateSourceCodePass2(name, cmd->commandSet);

		if (pass2 != "")
		{
			sprintf(line, "%s // 0x%04X", pass2.c_str(), cmd->cmdAddress);
			declarations[cmd->cmdAddress] = new Declaration(DeclarationAlignment::None, 8, line);

			sprintf(line, "extern %s _%s_set%04X_cmd%02X;\n", cmd->GetCommandCName().c_str(), name.c_str(), cmd->cmdSet & 0x00FFFFFF, cmd->cmdIndex, cmd->cmdID);
			externs[cmd->cmdAddress] = line;
		}
	}
}

ZRoomCommand* ZRoom::FindCommandOfType(RoomCommand cmdType)
{
	for (int i = 0; i < commands.size(); i++)
	{
		if (commands[i]->cmdID == cmdType)
			return commands[i];
	}

	return nullptr;
}

size_t ZRoom::GetDeclarationSizeFromNeighbor(int declarationAddress)
{
	int declarationIndex = -1;

	// Copy it into a vector.
	vector<pair<int32_t, Declaration*>> declarationKeysSorted(declarations.begin(), declarations.end());

	// Sort the vector according to the word count in descending order.
	sort(declarationKeysSorted.begin(), declarationKeysSorted.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

	for (int i = 0; i < declarationKeysSorted.size(); i++)
	{
		if (declarationKeysSorted[i].first == declarationAddress)
		{
			declarationIndex = i;
			break;
		}
	}

	if (declarationIndex != -1)
	{
		if (declarationIndex + 1 < declarationKeysSorted.size())
			return declarationKeysSorted[declarationIndex + 1].first - declarationKeysSorted[declarationIndex].first;
		else
			return rawData.size() - declarationKeysSorted[declarationIndex].first;
	}

	return 0;
}

size_t ZRoom::GetCommandSizeFromNeighbor(ZRoomCommand* cmd)
{
	int cmdIndex = -1;

	for (int i = 0; i < commands.size(); i++)
	{
		if (commands[i] == cmd)
		{
			cmdIndex = i;
			break;
		}
	}

	if (cmdIndex != -1)
	{
		if (cmdIndex + 1 < commands.size())
			return commands[cmdIndex + 1]->cmdAddress - commands[cmdIndex]->cmdAddress;
		else
			return rawData.size() - commands[cmdIndex]->cmdAddress;
	}

	return 0;
}

string ZRoom::GetSourceOutputHeader(string prefix)
{
	char line[2048];
	sourceOutput = "";

	for (ZRoomCommand* cmd : commands)
		sourceOutput += cmd->GenerateExterns();

	sourceOutput += "\n";

	// Copy it into a vector.
	vector<pair<int32_t, string>> externsKeysSorted(externs.begin(), externs.end());

	// Sort the vector according to the word count in descending order.
	sort(externsKeysSorted.begin(), externsKeysSorted.end(), [](const auto& lhs, const auto& rhs)
	{
		return lhs.first < rhs.first;
	});

	// Print out the vector.
	for (pair<int32_t, string> item : externsKeysSorted)
	{
		sourceOutput += item.second;
	}

	sourceOutput += "\n";

	return sourceOutput;
}

string ZRoom::GetSourceOutputCode(std::string prefix)
{
	sourceOutput = "";

	sourceOutput += "#include <z64.h>\n";
	sourceOutput += "#include <segment_symbols.h>\n";
	sourceOutput += "#include <command_macros_base.h>\n";
	sourceOutput += "#include <z64cutscene_commands.h>\n";
	sourceOutput += "#include <variables.h>\n";

	sourceOutput += StringHelper::Sprintf("#include \"%s.h\"\n", name.c_str());

	if (scene != nullptr)
	{
		sourceOutput += StringHelper::Sprintf("#include \"%s.h\"\n", scene->GetName().c_str());
	}

	sourceOutput += "\n";

	ProcessCommandSets();

	// Check for texture intersections
	{
		string defines = "";
		if (textures.size() != 0)
		{
			vector<pair<uint32_t, ZTexture*>> texturesSorted(textures.begin(), textures.end());

			sort(texturesSorted.begin(), texturesSorted.end(), [](const auto& lhs, const auto& rhs)
			{
				return lhs.first < rhs.first;
			});

			for (int i = 0; i < texturesSorted.size() - 1; i++)
			{
				int texSize = textures[texturesSorted[i].first]->GetRawDataSize();

				if ((texturesSorted[i].first + texSize) > texturesSorted[i + 1].first)
				{
					int intersectAmt = (texturesSorted[i].first + texSize) - texturesSorted[i + 1].first;

					defines += StringHelper::Sprintf("#define _%s_tex_%08X ((u32)_%s_tex_%08X + 0x%08X)\n", prefix.c_str(), texturesSorted[i + 1].first, prefix.c_str(),
						texturesSorted[i].first, texturesSorted[i + 1].first - texturesSorted[i].first);

					//int nSize = textures[texturesSorted[i].first]->GetRawDataSize();

					declarations.erase(texturesSorted[i + 1].first);
					externs.erase(texturesSorted[i + 1].first);
					textures.erase(texturesSorted[i + 1].first);
					texturesSorted.erase(texturesSorted.begin() + i + 1);

					//textures.erase(texturesSorted[i + 1].first);

					i--;
				}
			}
		}

		externs[0xFFFFFFFF] = defines;
	}

	for (pair<int32_t, ZTexture*> item : textures)
	{
		string declaration = "";

		declaration += item.second->GetSourceOutputCode(prefix);
		declarations[item.first] = new Declaration(DeclarationAlignment::None, item.second->GetRawDataSize(), item.second->GetSourceOutputCode(name));
	}

	// Copy it into a vector.
	vector<pair<int32_t, Declaration*>> declarationKeysSorted(declarations.begin(), declarations.end());

	// Sort the vector according to the word count in descending order.
	sort(declarationKeysSorted.begin(), declarationKeysSorted.end(), [](const auto& lhs, const auto& rhs)
	{
		return lhs.first < rhs.first;
	});

	int lastPtr = 0;

	// Account for padding/alignment
	int lastAddr = 0;

	for (pair<int32_t, Declaration*> item : declarationKeysSorted)
	{
		int alignment = (item.second->alignment == DeclarationAlignment::Align16) ? 16 :
			            (item.second->alignment == DeclarationAlignment::Align8) ? 8 :
			            (item.second->alignment == DeclarationAlignment::Align4) ? 4 :
			            1;

		while (declarations[item.first]->size % alignment != 0)
		{
			declarations[item.first]->size++;
		}

		if (lastAddr != 0)
		{
			if (item.second->alignment == DeclarationAlignment::Align16)
			{
				int lastAddrSizeTest = declarations[lastAddr]->size;
				int curPtr = lastAddr + declarations[lastAddr]->size;

				while (curPtr % 4 != 0)
				{
					declarations[lastAddr]->size++;
					//declarations[item.first]->size++;
					curPtr++;
				}

				/*while (curPtr % 16 != 0)
				{
					char buffer[2048];

					sprintf(buffer, "static u32 align%02X = 0;\n", curPtr);
					declarations[item.first]->text = buffer + declarations[item.first]->text;

					declarations[lastAddr]->size += 4;
					curPtr += 4;
				}*/
			}
			else if (item.second->alignment == DeclarationAlignment::Align8)
				{
					int curPtr = lastAddr + declarations[lastAddr]->size;

					while (curPtr % 4 != 0)
					{
						declarations[lastAddr]->size++;
						//item.second->size++;
						//declarations[item.first]->size++;
						curPtr++;
					}

					while (curPtr % 8 != 0)
					{
						char buffer[2048];

						sprintf(buffer, "static u32 align%02X = 0;\n", curPtr);
						declarations[item.first]->text = buffer + declarations[item.first]->text;

						declarations[lastAddr]->size += 4;
						//item.second->size += 4;
						//declarations[item.first]->size += 4;
						curPtr += 4;
					}
				}
		}

		if (item.second->padding == DeclarationPadding::Pad16)
		{
			int curPtr = item.first + item.second->size;

			while (curPtr % 4 != 0)
			{
				item.second->size++;
				curPtr++;
			}

			while (curPtr % 16 != 0)
			{
				char buffer[2048];

				sprintf(buffer, "static u32 pad%02X = 0;\n", curPtr);
				declarations[item.first]->text += buffer;

				item.second->size += 4;
				curPtr += 4;
			}
		}

		//sourceOutput += declarations[item.first]->text + "\n";

		lastAddr = item.first;
	}

	// Handle for unaccounted data
	lastAddr = 0;
	for (pair<int32_t, Declaration*> item : declarationKeysSorted)
	{
		if (lastAddr != 0)
		{
			if (lastAddr + declarations[lastAddr]->size > item.first)
			{
				// UH OH!
				int bp = 0;
			}

			if (lastAddr + declarations[lastAddr]->size != item.first)
			{
				int diff = item.first - (lastAddr + declarations[lastAddr]->size);

				string src = "";

				src += StringHelper::Sprintf("static u8 unaccounted%04X[] = \n{\n\t", lastAddr + declarations[lastAddr]->size);

				for (int i = 0; i < diff; i++)
				{
					src += StringHelper::Sprintf("0x%02X, ", rawData[lastAddr + declarations[lastAddr]->size + i]);

					if (i % 16 == 15)
						src += "\n\t";
				}

				src += "\n};\n";

				declarations[lastAddr + declarations[lastAddr]->size] = new Declaration(DeclarationAlignment::None, diff, src);
			}
		}

		lastAddr = item.first;
	}

	// TODO: THIS CONTAINS REDUNDANCIES. CLEAN THIS UP!
	if (lastAddr + declarations[lastAddr]->size < rawData.size())
	{
		int diff = (int)(rawData.size() - (lastAddr + declarations[lastAddr]->size));

		string src = "";

		src += StringHelper::Sprintf("static u8 unaccounted%04X[] = \n{\n\t", lastAddr + declarations[lastAddr]->size);

		for (int i = 0; i < diff; i++)
		{
			src += StringHelper::Sprintf("0x%02X, ", rawData[lastAddr + declarations[lastAddr]->size + i]);

			if (i % 16 == 15)
				src += "\n\t";
		}

		src += "\n};\n";

		declarations[lastAddr + declarations[lastAddr]->size] = new Declaration(DeclarationAlignment::None, diff, src);
	}

	// Print out our declarations
	declarationKeysSorted = vector<pair<int32_t, Declaration*>>(declarations.begin(), declarations.end());
	sort(declarationKeysSorted.begin(), declarationKeysSorted.end(), [](const auto& lhs, const auto& rhs)
	{
		return lhs.first < rhs.first;
	});

	for (pair<int32_t, Declaration*> item : declarationKeysSorted)
	{
		sourceOutput += item.second->text + "\n";
	}

	sourceOutput += "\n";

	return sourceOutput;
}

vector<uint8_t> ZRoom::GetRawData()
{
	return rawData;
}

int ZRoom::GetRawDataSize()
{
	int32_t size = 0;

	for (ZRoomCommand* cmd : commands)
		size += cmd->GetRawDataSize();

	return size;
}

Declaration::Declaration(DeclarationAlignment nAlignment, uint32_t nSize, string nText)
{
	alignment = nAlignment;
	padding = DeclarationPadding::None;
	size = nSize;
	text = nText;
}

Declaration::Declaration(DeclarationAlignment nAlignment, DeclarationPadding nPadding, uint32_t nSize, string nText)
{
	alignment = nAlignment;
	padding = nPadding;
	size = nSize;
	text = nText;
}

CommandSet::CommandSet(int32_t nAddress)
{
	address = nAddress;
	commandCount = 9999999;
}

CommandSet::CommandSet(int32_t nAddress, int32_t nCommandCount)
{
	address = nAddress;
	commandCount = nCommandCount;
}