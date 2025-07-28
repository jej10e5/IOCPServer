#pragma comment(lib, "mswsock.lib")
#include "pch.h"
#include "Listener.h"
#include "SessionManager.h"
#include "IocpContext.h"
#include "IocpCore.h"
bool Listener::Init(UINT16 _port)
{
	// ���� ���� �� �ʱ�ȭ
	// WSA_FLAG_OVERLAPPED �������� ������ AcceptEx(), WSARecv() ��� �Ұ�
	m_ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_ListenSocket == INVALID_SOCKET)
	{
		ERROR_LOG("���� ���� ���� ���� : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}
	// �� ������ �ѱ�� OS���� �ش� �ڵ�� Ŀ�� ��ü�� ������ I/O����
	// !!������ Ŀ�� ���ҽ��� ����ϹǷ� �ݵ�� �ݾƾ���!!
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);	// ��Ʈ ����
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� ip ���

	// m_ListenSocket ���Ͽ� Ư�� IP �ּҿ� ��Ʈ ��ȣ�� ��������� �����ؼ�
	// os�� �� ���Ͽ� ���� �� �� �ֵ��� ���
	// �� ip/port ������ ���� ����Ұž� ��� os�� ����
	if (::bind(m_ListenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ERROR_LOG("bind ���� : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}

	// 2. listen ȣ��
	// �� �ּҷ� ������ ���� ��û�� ���� �غ� �Ǿ� �ִٰ� os�� �˷���
	if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		ERROR_LOG("listen ���� : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD bytes = 0;

	
	// AcceptEx ������ ��������
	int result = WSAIoctl(
		m_ListenSocket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&bytes,
		NULL,
		NULL
	);
	// Ȯ�� �Լ��� ���� �����͸� ����
	// AcceptEx() ���� ȣ������ �ʰ� �Լ� �����͸� ������ ����
	// : Winsock�� Ȯ���Լ��� �������� Windows API�� ��ϵ� �Լ��� �ƴϱ� ����.
	//	��, ������ Ÿ�ӿ��� AcceptEx()�� ��Ȯ�� �ּҸ� �� �� ����. ��ũ ������ �Լ� �����͸� ���;���.

	if (result == SOCKET_ERROR)
	{
		ERROR_LOG("AcceptEx �Լ� �ε� ���� : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}

	// Listen ���� IOCP ���
	// os���� � ��û�� ������ �̺�Ʈ�� ���� iocp �ڵ� ��� �ϴ� �κ�
	IocpCore& iocpCore = IocpCore::GetInstance();
	iocpCore.RegisterSocket(m_ListenSocket, 0);

	return true;

}

bool Listener::PostAccept()
{
	SessionManager& sessionManager = SessionManager::GetInstance();
	Session* pSession = sessionManager.GetEmptySession();
	SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	pSession->m_Socket = clientSocket;
	
	//g_IocpCore.RegisterSocket(clientSocket, reinterpret_cast<ULONG_PTR>(pSession));

	IocpContext* pContext = new IocpContext();
	ZeroMemory(pContext, sizeof(IocpContext));
	pContext->eOperation = IocpOperation::ACCEPT;
	pContext->pSession = pSession;
	DWORD dwReceived = 0;

	//iocpť�� accept�̺�Ʈ �ѱ�
	//m_lpfnAcceptEx�� �Լ� �����ͷ� ����
	// Ŭ���̾�Ʈ�� �����ؿ� ������ OS���� �񵿱� ���� ������ ��û
	BOOL result = m_lpfnAcceptEx(
		m_ListenSocket,				// ���� ���� (���ſ�)
		clientSocket,				// ����� Ŭ���̾�Ʈ ����
		pContext->cBuffer,			// �ּ� ������ ������ ����
		0,							// ����� ������ ���� ũ�� (���� 0)
		sizeof(SOCKADDR_IN) + 16,	// ���� �ּ� ũ��
		sizeof(SOCKADDR_IN) + 16,	// ���� �ּ� ũ��
		&dwReceived,				// (���� ��� ����)
		pContext					// OVERLAPPED ����ü
	);

	if (result == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
	{
		ERROR_LOG("AcceptEx ���� : " << WSAGetLastError());
		closesocket(clientSocket); // ���� �ݰ�
		SessionManager& sessionManager = SessionManager::GetInstance();
		sessionManager.Release(pSession); // ���� ��ȯ
		delete pContext;
		return false;
	}

	LOG("���� ���� ����");
	return true;

}
