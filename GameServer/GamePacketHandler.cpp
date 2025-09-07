#include "GamePacketHandler.h"
#include "ClientSession.h"
#include "ServerSession.h"


void GamePacketHandler::Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	DCP_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}

void GamePacketHandler::Handle_DBEco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	SP_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}



void GamePacketHandler::Handle_Chat(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	SP_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());


	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}

void GamePacketHandler::Handle_Login(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	SP_LOGIN sendPkt;
	

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}

