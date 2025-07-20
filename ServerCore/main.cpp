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
		ERROR_LOG("WSAStartup ����");
		return;
	}

	// 1. listen socket ���� �� bind
	NetworkManager& networkManager = NetworkManager::GetInstance();
	networkManager.Init();
}

int main()
{
	// ���� Ǯ �ʱ�ȭ
	SessionManager& sessionManager = SessionManager::GetInstance();
	sessionManager.Init();

	// IOCP �ʱ�ȭ
	IocpCore& iocpCore = IocpCore::GetInstance();
	if (!iocpCore.Initialize())
	{
		ERROR_LOG("IOCP �ʱ�ȭ ����");
		return -1;
	}

	// accept ����
	std::thread acceptThread(StartAcceptLoop);
	acceptThread.detach();

	// IOCP ������ Run
	iocpCore.Run();


	while (true)
	{
		Sleep(1000);
	}

	return 0;

}