#include "GatePacketHandler.h"
#include "ClientSession.h"

void GatePacketHandler::SendToGame(Session* _pSession, PacketHeader* _pHeader)
{
	ClientSession* pClientSession = dynamic_cast<ClientSession*>(_pSession);
	if (!pClientSession)
	{
		ERROR_LOG("������ ClientSession���� ĳ���õ��� �ʽ��ϴ�.");
		return;
	}

	ServerSession* pGameServer = pClientSession->GetGameServer();
	if (pGameServer)
	{
		// ���� ������ ��Ŷ ����
		pGameServer->SendPacket(reinterpret_cast<const char*>(_pHeader), _pHeader->size);
	}
	else
	{
		ERROR_LOG("���� ������ nullptr�Դϴ�.");
	}


	// ���� ������ ��Ŷ ����
	if (_pSession && _pHeader)
	{
		_pSession->SendPacket(reinterpret_cast<const char*>(_pHeader), _pHeader->size);
	}
	else
	{
		ERROR_LOG("���� �Ǵ� ��Ŷ ����� nullptr�Դϴ�.");
	}

}

void GatePacketHandler::Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// ���� ��Ŷ ����
	PACKET_MSG_ECHO sendPkt;
	strcpy_s(sendPkt._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendPkt), sendPkt._header.size);
}

