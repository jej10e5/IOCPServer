#include "../ServerCommon/pch.h"
#include "Session.h"
#include "PacketID.h"
#include "PacketDispatcher.h"

static void Handle_Eco_Gate(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// ���� ��Ŷ ����
	PACKET_MSG_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}


REGISTER_HANDLER(CM_ECHO, Handle_Eco_Gate);