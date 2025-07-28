#pragma comment(lib, "mswsock.lib")
#include "pch.h"
#include "Listener.h"
#include "SessionManager.h"
#include "IocpContext.h"
#include "IocpCore.h"
bool Listener::Init(UINT16 _port)
{
	// 소켓 생성 및 초기화
	// WSA_FLAG_OVERLAPPED 설정하지 않으면 AcceptEx(), WSARecv() 사용 불가
	m_ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_ListenSocket == INVALID_SOCKET)
	{
		ERROR_LOG("리슨 소켓 생성 실패 : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}
	// 이 소켓을 넘기면 OS에서 해당 핸들로 커널 객체를 참조해 I/O수행
	// !!소켓은 커널 리소스를 사용하므로 반드시 닫아야함!!
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);	// 포트 설정
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 ip 허용

	// m_ListenSocket 소켓에 특정 IP 주소와 포트 번호를 명시적으로 지정해서
	// os가 이 소켓에 접근 할 수 있도록 등록
	// 이 ip/port 조합은 내가 사용할거야 라고 os에 선언
	if (::bind(m_ListenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ERROR_LOG("bind 실패 : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}

	// 2. listen 호출
	// 그 주소로 들어오는 연결 요청을 받을 준비가 되어 있다고 os에 알려줌
	if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		ERROR_LOG("listen 실패 : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD bytes = 0;

	
	// AcceptEx 포인터 가져오기
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
	// 확장 함수에 대한 포인터를 얻어옴
	// AcceptEx() 직접 호출하지 않고 함수 포인터를 얻어오는 이유
	// : Winsock의 확장함수로 정식으로 Windows API에 등록된 함수가 아니기 때문.
	//	즉, 컴파일 타임에는 AcceptEx()의 정확한 주소를 알 수 없음. 링크 오류로 함수 포인터를 얻어와야함.

	if (result == SOCKET_ERROR)
	{
		ERROR_LOG("AcceptEx 함수 로딩 실패 : " << WSAGetLastError());
		closesocket(m_ListenSocket);
		return false;
	}

	// Listen 소켓 IOCP 등록
	// os에서 어떤 요청이 끝나면 이벤트를 보낼 iocp 핸들 등록 하는 부분
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

	//iocp큐에 accept이벤트 넘김
	//m_lpfnAcceptEx는 함수 포인터로 얻어옴
	// 클라이언트가 접속해올 때까지 OS에게 비동기 연결 수락을 요청
	BOOL result = m_lpfnAcceptEx(
		m_ListenSocket,				// 리슨 소켓 (수신용)
		clientSocket,				// 연결될 클라이언트 소켓
		pContext->cBuffer,			// 주소 정보를 저장할 버퍼
		0,							// 사용자 데이터 버퍼 크기 (보통 0)
		sizeof(SOCKADDR_IN) + 16,	// 로컬 주소 크기
		sizeof(SOCKADDR_IN) + 16,	// 원격 주소 크기
		&dwReceived,				// (거의 사용 안함)
		pContext					// OVERLAPPED 구조체
	);

	if (result == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
	{
		ERROR_LOG("AcceptEx 실패 : " << WSAGetLastError());
		closesocket(clientSocket); // 소켓 닫고
		SessionManager& sessionManager = SessionManager::GetInstance();
		sessionManager.Release(pSession); // 세션 반환
		delete pContext;
		return false;
	}

	LOG("서버 리슨 시작");
	return true;

}
