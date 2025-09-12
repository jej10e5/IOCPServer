#include "pch.h"
#include "Session.h"
#include "IocpContext.h"
#include "NetworkManager.h"
#include "IocpCore.h"
#include "PacketDispatcher.h"
#include "SessionManager.h"
void Session::Init(SessionType _eType)
{
	m_RecvBuffer.Clear();
	m_AcceptSocket = INVALID_SOCKET;
	m_ListenSocket = INVALID_SOCKET;
	m_eSessionType = _eType;
	m_ui64SessionId = 0;
}
bool Session::Recv()
{
   // 1. IocpContext 생성
	IocpContext* pContext = new IocpContext;
	ZeroMemory(pContext, sizeof(IocpContext));
   // 3. operation을 IocpOperation::RECV으로 설정
	pContext->eOperation = IocpOperation::RECV; // 받았음
	pContext->pSession = this;
	pContext->tWsaBuf.buf = pContext->cBuffer; // 버퍼의 주소값
	pContext->tWsaBuf.len = MAX_BUFFER_SIZE; // 버퍼 사이즈 지정해주기

	DWORD dwFlags = 0;
	DWORD dwRecvBytes = 0;

	if (m_AcceptSocket == INVALID_SOCKET)
	{
		ERROR_LOG("Recv 실패 : m_Socket이 INVALID_SOCKET상태");
		return false;
	}

	//LOG("WSARecv 버퍼 주소: " << static_cast<void*>(pContext->tWsaBuf.buf));
	//LOG("WSARecv OVERLAPPED 주소: " << static_cast<void*>(pContext));
   // 2. WSABUF 설정
   // 4. WSARecv 호출
	INT32 i32result = WSARecv(m_AcceptSocket, &pContext->tWsaBuf, 1, &dwRecvBytes, &dwFlags, pContext, nullptr);

	// 5. 실패 시 처리 (WSA_IO_PENDING은 정상으로 간주)
	if (i32result == SOCKET_ERROR)
	{
		INT32 i32Error = WSAGetLastError();
		if (i32Error != WSA_IO_PENDING)
		{
			ERROR_LOG("WSARecv 실패 : " << i32Error);
			delete pContext;
			return false;
		}
	}

	return true;
}
bool Session::Send(const char* _pData, INT32 _i32Len)
{
	std::lock_guard<std::mutex> guard(m_SendLock);
	if (m_SendQueue.empty()) 
	{  // 보낼 게 없으면 종료
		m_bIsSending = false;
		return true;
		
	}
	auto * pContext = new IocpContext();
	ZeroMemory(pContext, sizeof(IocpContext));
	pContext->eOperation = IocpOperation::SEND;
	pContext->pSession = this;

	auto & item = m_SendQueue.front();
	char* p = item.buf->data() + item.off;
	size_t n = item.buf->size() - item.off;
	pContext->tWsaBuf.buf = p;
	pContext->tWsaBuf.len = static_cast<ULONG>(n);

	DWORD flags = 0;
	DWORD bytes = 0;
	int iRet = WSASend(m_AcceptSocket, &pContext->tWsaBuf, 1, &bytes, flags, pContext, NULL);

	if (iRet == SOCKET_ERROR)
	{
		int i32Error = WSAGetLastError();
		if (i32Error != WSA_IO_PENDING)
		{
			ERROR_LOG("WSASend 실패 : " << i32Error);
			delete pContext;
			Disconnect();
			return false;
		}
	}
	return true;
}



void Session::OnSendCompleted(IocpContext* _pContext, DWORD _dwSendLen)
{
	if (_dwSendLen == 0)
	{
		// 상대방 종료로 간주
		Disconnect();
		return;
	}

	bool needPost = false;

	{
		std::lock_guard<std::mutex> guard(m_SendLock);

		if (m_SendQueue.empty()) {
			m_bIsSending = false;
			return;
		}

		// 진행 상황 업데이트
		auto& item = m_SendQueue.front();
		item.off += _dwSendLen;

		if (item.off >= item.buf->size()) {
			// 이번 버퍼 송신 완료 → 팝
			m_SendQueue.pop();
			if (!m_SendQueue.empty()) {
				// 아직 남아있으면 다음 걸 이어서 포스트해야 함
				needPost = true;
			}
			else {
				// 더 보낼 게 없으면 플래그 내림
				m_bIsSending = false;
			}
		}
		else {
			// 이번 버퍼가 아직 남아있음 → 이어서 포스트
			needPost = true;
		}
	} // ← 여기서 락 해제됨

	if (needPost) {
		Send(nullptr, 0);  // 다음 WSASend 포스트 (락 없이 진입)
	}

}

void Session::OnAcceptCompleted(IocpContext* _pContext)
{

	NetworkManager& networkManager = NetworkManager::GetInstance();
	SOCKET listenSock = m_ListenSocket;
	// SO_UPDATE_ACCEPT_CONTEXT 설정하는 이유
	//  - 커널에게 이 소켓은 이 리슨 소켓을 통해 accept된거라고 알려주는 역할
	//  - 그래야 커널이 해당 소켓 리소스를 정리할 타이밍을 제대로 계산
	INT32 i32OptResult = setsockopt(m_AcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSock, sizeof(SOCKET));

	if (i32OptResult == SOCKET_ERROR)
	{
		ERROR_LOG("setsockopt  SO_UPDATE_ACCEPT_CONTEXT  실패 : " << WSAGetLastError());

		closesocket(m_AcceptSocket);
		g_SessionManager.Release(m_eSessionType, this);
		return;
	}

	IocpCore& iocpCore = IocpCore::GetInstance();
	if (!iocpCore.RegisterSocket(m_AcceptSocket, reinterpret_cast<ULONG_PTR>(this)))
	{
		ERROR_LOG("소켓 IOCP 등록 실패");
		closesocket(m_AcceptSocket);
		g_SessionManager.Release(m_eSessionType, this);
		return;
	}

	m_ui64SessionId = g_SessionManager.RegisterActive(this);
	LOG("클라이언트 접속 완료 (token = " << m_ui64SessionId << ")");


	if (!Recv())
	{
		ERROR_LOG("WSARecv 실패");
	}
	networkManager.AcceptListener(m_eSessionType);
}

// Session.cpp
void Session::Disconnect()
{
	bool expected = false;
	if (!m_bDisconnected.compare_exchange_strong(expected, true))
		return; // 이미 종료 처리 중

	const UINT64 sid = GetSessionId(); // ← 스냅샷

	// 1) I/O 차단
	if (m_AcceptSocket != INVALID_SOCKET) {
		shutdown(m_AcceptSocket, SD_BOTH);
		closesocket(m_AcceptSocket);
		m_AcceptSocket = INVALID_SOCKET; // ← 플래그 정리
	}
	LOG("클라이언트 접속 종료 - token : " << sid);

	// 2) 먼저 도메인 통지
	if (m_onDisconnect) {
		auto cb = m_onDisconnect;     // 안전하게 복사
		m_onDisconnect = nullptr;     // 재진입 방지
		cb(sid);
	}

	// 3) 마지막에 세션 매니저 정리
	auto& sessionManager = SessionManager::GetInstance();
	sessionManager.UnregisterActive(this);
	sessionManager.Release(m_eSessionType, this);
}

void Session::SendPacket(const char* _pData, INT32 _i32Len)
{
	auto pkt = std::make_shared<std::vector<char>>(_pData, _pData + _i32Len);
	{
		std::lock_guard<std::mutex> guard(m_SendLock);
		m_SendQueue.push(SendItem{ pkt, 0 });
		if (m_bIsSending)
			return;             // 이미 전송 중이면 큐에만 쌓고 끝
		m_bIsSending = true;    // 전송 시작 플래그 on
	}
	// 큐의 front를 기준으로 첫 WSASend 포스트
	Send(nullptr, 0);           // (인자는 무시; front 기반으로 보냄)

}


