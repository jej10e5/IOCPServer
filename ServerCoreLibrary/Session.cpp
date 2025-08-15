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
	m_Socket = INVALID_SOCKET;
	m_eSessionType = _eType;
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

	if (m_Socket == INVALID_SOCKET)
	{
		ERROR_LOG("Recv 실패 : m_Socket이 INVALID_SOCKET상태");
		return false;
	}

	LOG("WSARecv 버퍼 주소: " << static_cast<void*>(pContext->tWsaBuf.buf));
	LOG("WSARecv OVERLAPPED 주소: " << static_cast<void*>(pContext));
   // 2. WSABUF 설정
   // 4. WSARecv 호출
	INT32 i32result = WSARecv(m_Socket, &pContext->tWsaBuf, 1, &dwRecvBytes, &dwFlags, pContext, nullptr);

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
	// 클라이언트에 데이터 비동기 방식으로 보내는 함수

	// 1. IocpContext 생성	
	IocpContext* pContext = new IocpContext();
	ZeroMemory(pContext, sizeof(IocpContext));

	// 2. eOperation = SEND
	pContext->eOperation = IocpOperation::SEND;
	pContext->pSession = this;

	// 3. WSABUF 설정 (buf, len)
	memcpy(pContext->cBuffer, _pData, _i32Len);
	pContext->tWsaBuf.buf = pContext->cBuffer;
	pContext->tWsaBuf.len = _i32Len;

	DWORD dwFlag=0;
	DWORD dwSentBytes=0;

	// 4. WSASend 호출
	INT32 i32Result = WSASend(m_Socket, &pContext->tWsaBuf, 1, &dwSentBytes, 0, pContext, nullptr);

	if (i32Result == SOCKET_ERROR)
	{
		// 5. WSA_IO_PENDING 예외 처리
		INT32 i32Error = WSAGetLastError();
		if (i32Error != WSA_IO_PENDING)
		{
			ERROR_LOG("WSASend 실패 : " << i32Error);
			delete pContext;
			return false;
		}
	}

	return true;
}



void Session::OnSendCompleted(IocpContext* _pContext, DWORD _dwSendLen)
{
	if (_dwSendLen == 0)
	{
		// 상대방이 소켓을 종료함
		Disconnect();
	}
	else
	{
		std::lock_guard<std::mutex> guard(m_SendLock);
		if (!m_SendQueue.empty())
		{
			m_SendQueue.pop(); // 현재 메세지 제거
		}

		// 다음 데이터 있는지 체크
		if (!m_SendQueue.empty())
		{
			// 있으면 send 호출
			auto& next = m_SendQueue.front();
			Send(next.data(), static_cast<INT32>(next.size()));
			return;
		}

		// 없으면 풀어주기
		m_bIsSending = false;

		LOG("Send 완료: " << _dwSendLen << " 바이트 전송됨");
	}

}

void Session::OnAcceptCompleted(IocpContext* _pContext)
{
	NetworkManager& networkManager = NetworkManager::GetInstance();
	SOCKET listenSock = m_Socket;
	// SO_UPDATE_ACCEPT_CONTEXT 설정하는 이유
	//  - 커널에게 이 소켓은 이 리슨 소켓을 통해 accept된거라고 알려주는 역할
	//  - 그래야 커널이 해당 소켓 리소스를 정리할 타이밍을 제대로 계산
	INT32 i32OptResult = setsockopt(m_Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSock, sizeof(SOCKET));
	SessionManager& sessionManager = SessionManager::GetInstance();
	if (i32OptResult == SOCKET_ERROR)
	{
		ERROR_LOG("setsockopt  SO_UPDATE_ACCEPT_CONTEXT  실패 : " << WSAGetLastError());

		closesocket(m_Socket);
		sessionManager.Release(m_eSessionType, this);
		delete _pContext;
		return;
	}

	IocpCore& iocpCore = IocpCore::GetInstance();
	if (!iocpCore.RegisterSocket(m_Socket, reinterpret_cast<ULONG_PTR>(this)))
	{
		ERROR_LOG("소켓 IOCP 등록 실패");
		closesocket(m_Socket);
		sessionManager.Release(m_eSessionType, this);
		delete _pContext;
		return;
	}
	LOG("클라이언트 접속 완료");

	//SendPacket("Hello1", 6);
	//SendPacket("Hello2", 6);
	//SendPacket("Hello3", 6);

	if (!Recv())
	{
		ERROR_LOG("WSARecv 실패");
	}
	networkManager.AcceptListener(m_eSessionType);
}

void Session::Disconnect()
{
	// 1. 중복 Disconnect 방지 (isConnected 같은 상태변수 있으면 체크)
	
	// 2. 소켓 닫기: closesocket(m_socket)
	closesocket(m_Socket);
	
	LOG("소켓 닫기");

	// 3. 세션 정리 (메모리 삭제 or 풀에 반납)
	SessionManager& sessionManager = SessionManager::GetInstance();
	sessionManager.Release(m_eSessionType, this);

}

void Session::SendPacket(const char* _pData, INT32 _i32Len)
{
	std::lock_guard<std::mutex> guard(m_SendLock);

	// 보낼 데이터 복사
	std::vector<char> vPacket(_pData, _pData + _i32Len);
	m_SendQueue.push(std::move(vPacket));

	if (!m_bIsSending)
	{
		m_bIsSending = true;
		auto& front = m_SendQueue.front();
		Send(front.data(), static_cast<INT32>(front.size()));
	}

}


