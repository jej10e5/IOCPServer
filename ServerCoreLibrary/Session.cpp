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
   // 1. IocpContext ����
	IocpContext* pContext = new IocpContext;
	ZeroMemory(pContext, sizeof(IocpContext));
   // 3. operation�� IocpOperation::RECV���� ����
	pContext->eOperation = IocpOperation::RECV; // �޾���
	pContext->pSession = this;
	pContext->tWsaBuf.buf = pContext->cBuffer; // ������ �ּҰ�
	pContext->tWsaBuf.len = MAX_BUFFER_SIZE; // ���� ������ �������ֱ�

	DWORD dwFlags = 0;
	DWORD dwRecvBytes = 0;

	if (m_Socket == INVALID_SOCKET)
	{
		ERROR_LOG("Recv ���� : m_Socket�� INVALID_SOCKET����");
		return false;
	}

	LOG("WSARecv ���� �ּ�: " << static_cast<void*>(pContext->tWsaBuf.buf));
	LOG("WSARecv OVERLAPPED �ּ�: " << static_cast<void*>(pContext));
   // 2. WSABUF ����
   // 4. WSARecv ȣ��
	INT32 i32result = WSARecv(m_Socket, &pContext->tWsaBuf, 1, &dwRecvBytes, &dwFlags, pContext, nullptr);

	// 5. ���� �� ó�� (WSA_IO_PENDING�� �������� ����)
	if (i32result == SOCKET_ERROR)
	{
		INT32 i32Error = WSAGetLastError();
		if (i32Error != WSA_IO_PENDING)
		{
			ERROR_LOG("WSARecv ���� : " << i32Error);
			delete pContext;
			return false;
		}
	}

	return true;
}

bool Session::Send(const char* _pData, INT32 _i32Len)
{
	// Ŭ���̾�Ʈ�� ������ �񵿱� ������� ������ �Լ�

	// 1. IocpContext ����	
	IocpContext* pContext = new IocpContext();
	ZeroMemory(pContext, sizeof(IocpContext));

	// 2. eOperation = SEND
	pContext->eOperation = IocpOperation::SEND;
	pContext->pSession = this;

	// 3. WSABUF ���� (buf, len)
	memcpy(pContext->cBuffer, _pData, _i32Len);
	pContext->tWsaBuf.buf = pContext->cBuffer;
	pContext->tWsaBuf.len = _i32Len;

	DWORD dwFlag=0;
	DWORD dwSentBytes=0;

	// 4. WSASend ȣ��
	INT32 i32Result = WSASend(m_Socket, &pContext->tWsaBuf, 1, &dwSentBytes, 0, pContext, nullptr);

	if (i32Result == SOCKET_ERROR)
	{
		// 5. WSA_IO_PENDING ���� ó��
		INT32 i32Error = WSAGetLastError();
		if (i32Error != WSA_IO_PENDING)
		{
			ERROR_LOG("WSASend ���� : " << i32Error);
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
		// ������ ������ ������
		Disconnect();
	}
	else
	{
		std::lock_guard<std::mutex> guard(m_SendLock);
		if (!m_SendQueue.empty())
		{
			m_SendQueue.pop(); // ���� �޼��� ����
		}

		// ���� ������ �ִ��� üũ
		if (!m_SendQueue.empty())
		{
			// ������ send ȣ��
			auto& next = m_SendQueue.front();
			Send(next.data(), static_cast<INT32>(next.size()));
			return;
		}

		// ������ Ǯ���ֱ�
		m_bIsSending = false;

		LOG("Send �Ϸ�: " << _dwSendLen << " ����Ʈ ���۵�");
	}

}

void Session::OnAcceptCompleted(IocpContext* _pContext)
{
	NetworkManager& networkManager = NetworkManager::GetInstance();
	SOCKET listenSock = m_Socket;
	// SO_UPDATE_ACCEPT_CONTEXT �����ϴ� ����
	//  - Ŀ�ο��� �� ������ �� ���� ������ ���� accept�ȰŶ�� �˷��ִ� ����
	//  - �׷��� Ŀ���� �ش� ���� ���ҽ��� ������ Ÿ�̹��� ����� ���
	INT32 i32OptResult = setsockopt(m_Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSock, sizeof(SOCKET));
	SessionManager& sessionManager = SessionManager::GetInstance();
	if (i32OptResult == SOCKET_ERROR)
	{
		ERROR_LOG("setsockopt  SO_UPDATE_ACCEPT_CONTEXT  ���� : " << WSAGetLastError());

		closesocket(m_Socket);
		sessionManager.Release(m_eSessionType, this);
		delete _pContext;
		return;
	}

	IocpCore& iocpCore = IocpCore::GetInstance();
	if (!iocpCore.RegisterSocket(m_Socket, reinterpret_cast<ULONG_PTR>(this)))
	{
		ERROR_LOG("���� IOCP ��� ����");
		closesocket(m_Socket);
		sessionManager.Release(m_eSessionType, this);
		delete _pContext;
		return;
	}
	LOG("Ŭ���̾�Ʈ ���� �Ϸ�");

	//SendPacket("Hello1", 6);
	//SendPacket("Hello2", 6);
	//SendPacket("Hello3", 6);

	if (!Recv())
	{
		ERROR_LOG("WSARecv ����");
	}
	networkManager.AcceptListener(m_eSessionType);
}

void Session::Disconnect()
{
	// 1. �ߺ� Disconnect ���� (isConnected ���� ���º��� ������ üũ)
	
	// 2. ���� �ݱ�: closesocket(m_socket)
	closesocket(m_Socket);
	
	LOG("���� �ݱ�");

	// 3. ���� ���� (�޸� ���� or Ǯ�� �ݳ�)
	SessionManager& sessionManager = SessionManager::GetInstance();
	sessionManager.Release(m_eSessionType, this);

}

void Session::SendPacket(const char* _pData, INT32 _i32Len)
{
	std::lock_guard<std::mutex> guard(m_SendLock);

	// ���� ������ ����
	std::vector<char> vPacket(_pData, _pData + _i32Len);
	m_SendQueue.push(std::move(vPacket));

	if (!m_bIsSending)
	{
		m_bIsSending = true;
		auto& front = m_SendQueue.front();
		Send(front.data(), static_cast<INT32>(front.size()));
	}

}


