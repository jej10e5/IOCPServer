#include "GatePacketHandler.h"
#include "ClientSession.h"
#include "ServerSession.h"

void GatePacketHandler::SendToGame(Session* _pSession, PacketHeader* _pHeader)
{
	ServerSession* pServerSession = dynamic_cast<ServerSession*>(_pSession);
	if (!pServerSession)
	{
		ERROR_LOG("������ ServerSession���� ĳ���õ��� �ʽ��ϴ�.");
		return;
	}

	if (pServerSession)
	{
		// ���� ������ ��Ŷ ����
		pServerSession->SendPacket(reinterpret_cast<const char*>(_pHeader), _pHeader->size);
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

