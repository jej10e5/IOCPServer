#include "pch.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "NetworkManager.h"
#include "IocpCore.h"

int main()
{
    // 1. Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ERROR_LOG("WSAStartup ����");
        return -1;
    }

    // 2. SessionManager�� ClientSession ���� ���� ����
    SessionManager::GetInstance().Init([]() {
        return new ClientSession();
        });

    // 3. IOCP Core �ʱ�ȭ
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP �ʱ�ȭ ����");
        return -1;
    }

    // 4. ��Ʈ��ũ ���ε� �� ����
    NetworkManager& network = NetworkManager::GetInstance();
    network.Init(7777);
    // 5. Accept ����
    network.AcceptListener();
   

    LOG("GateServer ���� ����");

    // 6. IOCP ���� ����
    iocp.Run();

    // 7. ���� ����
    WSACleanup();

    return 0;
}
