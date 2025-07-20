#include "pch.h"
#include "SessionManager.h"
#include "IocpCore.h"
#include "NetworkManager.h"
#include "PacketHandlers.h"

void StartAcceptLoop()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ERROR_LOG("WSAStartup 실패");
		return;
	}

	// 1. listen socket 생성 및 bind
	NetworkManager& networkManager = NetworkManager::GetInstance();
	networkManager.Init();
}

int main()
{
	// 세션 풀 초기화
	SessionManager& sessionManager = SessionManager::GetInstance();
	sessionManager.Init();

	// IOCP 초기화
	IocpCore& iocpCore = IocpCore::GetInstance();
	if (!iocpCore.Initialize())
	{
		ERROR_LOG("IOCP 초기화 실패");
		return -1;
	}

	// accept 루프
	std::thread acceptThread(StartAcceptLoop);
	acceptThread.detach();

	// IOCP 스레드 Run
	iocpCore.Run();


	while (true)
	{
		Sleep(1000);
	}

	return 0;

}