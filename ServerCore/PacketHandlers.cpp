#include "PacketDispatcher.h"
#include "PacketHandlers.h"
#include "Session.h"

void Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	PACKET_MSG_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}


REGISTER_HANDLER(PACKET_ID_ECHO, Handle_Eco);