#pragma once
#include "../ServerCommon/pch.h"
#include "PacketDispatcher.h"
#include "PacketID.h"
#include "../ServerCommon/Utils.h"
class Session;
class GamePacketHandler
{
public:
	static void Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size);
	static void Handle_DBEco(Session* _pSession, const char* _pData, UINT16 _ui16size);
	static void Handle_Chat(Session* _pSession, const char* _pData, UINT16 _ui16size);
	static void Handle_Login(Session* _pSession, const char* _pData, UINT16 _ui16size);
};

