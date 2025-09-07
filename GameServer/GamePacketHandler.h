#pragma once
#include "../ServerCommon/pch.h"
#include "PacketDispatcher.h"
#include "PacketID.h"
class Session;
class GamePacketHandler
{
public:
	static void Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size);
};

