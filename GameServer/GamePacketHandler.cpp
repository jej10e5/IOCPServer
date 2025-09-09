#include "GamePacketHandler.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "SessionManager.h"
#include "CUnitPC.h"
#include "GameManager.h"
void GamePacketHandler::Handle_Eco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	SP_ECHO sendMsg;
	strcpy_s(sendMsg._msg, msg.c_str());

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendMsg), sendMsg._header.size);
	
}

void GamePacketHandler::Handle_DBEco(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	std::string msg(_pData + PACKET_HEADER_SIZE, _ui16size - PACKET_HEADER_SIZE);
	LOG("echo : " << msg);

	// 응답 패킷 구성
	SP_ECHO sendMsg;
	strcpy_s(sendMsg._msg, msg.c_str());


	_pSession->SendPacket(reinterpret_cast<const char*>(&sendMsg), sendMsg._header.size);
}



void GamePacketHandler::Handle_Chat(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	if (_ui16size < sizeof(CP_LOGIN))
		return;

	const CP_CHAT* pMsg = reinterpret_cast<const CP_CHAT*>(_pData);
	if (pMsg == NULL)
		return;

	// 응답 패킷 구성
	SP_CHAT sendMsg;

	sendMsg._ui64id = _pSession->GetToken();
	auto pUser = g_GameManager.FindUserById(_pSession->GetToken());
	if (pUser == NULL)
		return;

	pUser->GetName(sendMsg._name);
	sendMsg._name[255] = 0;

	memcpy(sendMsg._chat, pMsg->_chat, 256 - 1);

	//_pSession->SendPacket(reinterpret_cast<const char*>(&sendMsg), sendMsg._header.size);
	SessionManager& sessionManager = SessionManager::GetInstance();
	sessionManager.BroadCastActive((char*)&sendMsg, sendMsg._header.size);
}

void GamePacketHandler::Handle_Login(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	if (_ui16size < sizeof(CP_LOGIN)) 
	{
		return;
	}
	const CP_LOGIN* pMsg = reinterpret_cast<const CP_LOGIN*>(_pData);
	if (pMsg == NULL)
		return;

	char name[256] = { 0 };                
	std::memcpy(name, pMsg->_name, 256 - 1);
	name[256 - 1] = '\0';

	g_GameManager.Login(_pSession->GetToken(), name);

	// 응답 패킷 구성
	SP_LOGIN sendMsg;

	sendMsg._ui64id = _pSession->GetToken();
	memcpy(sendMsg._name, pMsg->_name, sizeof(sendMsg._name));

	_pSession->SendPacket(reinterpret_cast<const char*>(&sendMsg), sendMsg._header.size);

}

