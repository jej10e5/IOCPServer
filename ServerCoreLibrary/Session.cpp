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

	if (m_AcceptSocket == INVALID_SOCKET)
	{
		ERROR_LOG("Recv ���� : m_Socket�� INVALID_SOCKET����");
		return false;
	}

	//LOG("WSARecv ���� �ּ�: " << static_cast<void*>(pContext->tWsaBuf.buf));
	//LOG("WSARecv OVERLAPPED �ּ�: " << static_cast<void*>(pContext));
   // 2. WSABUF ����
   // 4. WSARecv ȣ��
	INT32 i32result = WSARecv(m_AcceptSocket, &pContext->tWsaBuf, 1, &dwRecvBytes, &dwFlags, pContext, nullptr);

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
	std::lock_guard<std::mutex> guard(m_SendLock);
	if (m_SendQueue.empty()) 
	{  // ���� �� ������ ����
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
			ERROR_LOG("WSASend ���� : " << i32Error);
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
		// ���� ����� ����
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

		// ���� ��Ȳ ������Ʈ
		auto& item = m_SendQueue.front();
		item.off += _dwSendLen;

		if (item.off >= item.buf->size()) {
			// �̹� ���� �۽� �Ϸ� �� ��
			m_SendQueue.pop();
			if (!m_SendQueue.empty()) {
				// ���� ���������� ���� �� �̾ ����Ʈ�ؾ� ��
				needPost = true;
			}
			else {
				// �� ���� �� ������ �÷��� ����
				m_bIsSending = false;
			}
		}
		else {
			// �̹� ���۰� ���� �������� �� �̾ ����Ʈ
			needPost = true;
		}
	} // �� ���⼭ �� ������

	if (needPost) {
		Send(nullptr, 0);  // ���� WSASend ����Ʈ (�� ���� ����)
	}

}

void Session::OnAcceptCompleted(IocpContext* _pContext)
{

	NetworkManager& networkManager = NetworkManager::GetInstance();
	SOCKET listenSock = m_ListenSocket;
	// SO_UPDATE_ACCEPT_CONTEXT �����ϴ� ����
	//  - Ŀ�ο��� �� ������ �� ���� ������ ���� accept�ȰŶ�� �˷��ִ� ����
	//  - �׷��� Ŀ���� �ش� ���� ���ҽ��� ������ Ÿ�̹��� ����� ���
	INT32 i32OptResult = setsockopt(m_AcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSock, sizeof(SOCKET));

	if (i32OptResult == SOCKET_ERROR)
	{
		ERROR_LOG("setsockopt  SO_UPDATE_ACCEPT_CONTEXT  ���� : " << WSAGetLastError());

		closesocket(m_AcceptSocket);
		g_SessionManager.Release(m_eSessionType, this);
		return;
	}

	IocpCore& iocpCore = IocpCore::GetInstance();
	if (!iocpCore.RegisterSocket(m_AcceptSocket, reinterpret_cast<ULONG_PTR>(this)))
	{
		ERROR_LOG("���� IOCP ��� ����");
		closesocket(m_AcceptSocket);
		g_SessionManager.Release(m_eSessionType, this);
		return;
	}

	m_ui64SessionId = g_SessionManager.RegisterActive(this);
	LOG("Ŭ���̾�Ʈ ���� �Ϸ� (token = " << m_ui64SessionId << ")");


	if (!Recv())
	{
		ERROR_LOG("WSARecv ����");
	}
	networkManager.AcceptListener(m_eSessionType);
}

// Session.cpp
void Session::Disconnect()
{
	bool expected = false;
	if (!m_bDisconnected.compare_exchange_strong(expected, true))
		return; // �̹� ���� ó�� ��

	const UINT64 sid = GetSessionId(); // �� ������

	// 1) I/O ����
	if (m_AcceptSocket != INVALID_SOCKET) {
		shutdown(m_AcceptSocket, SD_BOTH);
		closesocket(m_AcceptSocket);
		m_AcceptSocket = INVALID_SOCKET; // �� �÷��� ����
	}
	LOG("Ŭ���̾�Ʈ ���� ���� - token : " << sid);

	// 2) ���� ������ ����
	if (m_onDisconnect) {
		auto cb = m_onDisconnect;     // �����ϰ� ����
		m_onDisconnect = nullptr;     // ������ ����
		cb(sid);
	}

	// 3) �������� ���� �Ŵ��� ����
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
			return;             // �̹� ���� ���̸� ť���� �װ� ��
		m_bIsSending = true;    // ���� ���� �÷��� on
	}
	// ť�� front�� �������� ù WSASend ����Ʈ
	Send(nullptr, 0);           // (���ڴ� ����; front ������� ����)

}


