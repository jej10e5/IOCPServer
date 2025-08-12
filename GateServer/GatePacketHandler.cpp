#include "GatePacketHandler.h"
#include "ClientSession.h"

void GatePacketHandler::SendToGame(Session* _pSession, PacketHeader* _pHeader)
{
	ClientSession* pClientSession = dynamic_cast<ClientSession*>(_pSession);
	if (!pClientSession)
	{
		ERROR_LOG("세션이 ClientSession으로 캐스팅되지 않습니다.");
		return;
	}

	ServerSession* pGameServer = pClientSession->GetGameServer();
	if (pGameServer)
	{
		// 게임 서버로 패킷 전송
		pGameServer->SendPacket(reinterpret_cast<const char*>(_pHeader), _pHeader->size);
	}
	else
	{
		ERROR_LOG("게임 서버가 nullptr입니다.");
	}


	// 게임 서버로 패킷 전송
	if (_pSession && _pHeader)
	{
		_pSession->SendPacket(reinterpret_cast<const char*>(_pHeader), _pHeader->size);
	}
	else
	{
		ERROR_LOG("세션 또는 패킷 헤더가 nullptr입니다.");
	}

}

void GatePacketHandler::Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	PACKET_MSG_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}

