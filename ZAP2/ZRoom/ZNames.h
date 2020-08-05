#pragma once

#include "ActorList.h"
#include "../Globals.h"
#include "ObjectList.h"
#include "RoomList.h"

#include <string>

class ZNames
{
public:
	static std::string GetRoomName(int offset)
	{
		if (Globals::Instance->game == ZGame::OOT)
			return RoomList[offset];
		else
			return RoomListMM[offset];

	}

	static std::string GetObjectName(int id)
	{
		if (Globals::Instance->game == ZGame::OOT)
			return ObjectList[id];
		else
			return ObjectListMM[id];
	}

	static std::string GetActorName(int id)
	{
		if (Globals::Instance->game == ZGame::OOT)
			return ActorList[id];
		else
			return ActorListMM[id];
	}

	static int GetNumActors()
	{
		if (Globals::Instance->game == ZGame::OOT)
			return sizeof(ActorList) / sizeof(ActorList[0]);
		else
			return sizeof(ActorListMM) / sizeof(ActorListMM[0]);
	}
};