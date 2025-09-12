#include "GamePacketHandler.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "SessionManager.h"
#include "CUnitPC.h"
#include "GameManager.h"
#include "DBManager.h"
#include "GameDispatcher.h"
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

	sendMsg._i64Unique = _pSession->GetSessionId();
	auto pUser = g_GameManager.FindUserById(_pSession->GetSessionId());
	if (pUser == NULL)
		return;

	pUser->GetName(sendMsg._id);
	memcpy(sendMsg._chat, pMsg->_chat, MAX_CHAT_LENGTH);
	std::wstring wname = Utf8ToWide(sendMsg._id);
	std::wstring wchat = Utf8ToWide(sendMsg._chat);
	std::wcout << L"chat - [" << wname << L"] : " << wchat << L"\n";

	//_pSession->SendPacket(reinterpret_cast<const char*>(&sendMsg), sendMsg._header.size);
	SessionManager& sessionManager = SessionManager::GetInstance();
	sessionManager.BroadCastActive((char*)&sendMsg, sendMsg._header.size);
}

void GamePacketHandler::Handle_Login(Session* _pSession, const char* _pData, UINT16 _ui16size)
{
	if (_ui16size < sizeof(CP_LOGIN))
		return;

	const CP_LOGIN* pMsg = reinterpret_cast<const CP_LOGIN*>(_pData);
	if (pMsg == nullptr)
		return;

	uint64_t sessionId = _pSession->GetSessionId();

	DBManager::Instance().Enqueue_CheckAccount(
		pMsg->_id, pMsg->_pw,
		[sessionId, pMsg](int unique)
		{
			// DB 결과는 워커 스레드에서 옴 → 게임 스레드 큐로 마샬링
			GameDispatcher::GetInstance().PostToGameThread([sessionId, unique, pMsg]
				{
					Session* pSession = g_SessionManager.FindByToken(sessionId);
					if (!pSession) return;

					g_GameManager.Login(pSession, unique, pMsg->_id);

					SP_LOGIN rsp{};
					rsp._i32Result = (unique == 0 ? -1 : 0);
					rsp._i64Unique = sessionId;

					// 토큰 값을 문자열로 넣어주기 (예시: name 필드에)
					memcpy(rsp._id, pMsg->_id, sizeof(rsp._id));
					memcpy(rsp._pw, pMsg->_pw, sizeof(rsp._pw));
					rsp._i64Unique = unique;
					pSession->SendPacket(reinterpret_cast<const char*>(&rsp), rsp._header.size);
				});
		}
	);
}
