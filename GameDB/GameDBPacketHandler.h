#pragma once
#include "../ServerCommon/pch.h"
#include "PacketDispatcher.h"
#include "PacketID.h"
class Session;
class GameDBPacketHandler
{
public:
	static void Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size);
	static void Handle_Chat(Session* _pSession, const char* _pData, UINT16 _ui16size);
	static void Handle_Login(Session* _pSession, const char* _pData, UINT16 _ui16size);
};

