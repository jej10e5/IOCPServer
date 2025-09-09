#include "NetworkManager.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "IocpCore.h"
#include "GamePacketHandler.h"


void InitGateHandlers()
{
    // ��Ŷ �ڵ鷯 ���
    REGISTER_HANDLER(CM_ECHO, GamePacketHandler::Handle_Eco);
    REGISTER_HANDLER(CM_LOGIN, GamePacketHandler::Handle_Login);
    REGISTER_HANDLER(CM_CHAT, GamePacketHandler::Handle_Chat);
}

int main()
{
    // 1. IOCP Core �ʱ�ȭ
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP �ʱ�ȭ ����");
        return -1;
    }

    //2. SessionManager�� Session ���� ���� ����
    //����Ʈ������ Ŭ���̾�Ʈ ���ǰ� ���� ������ ����
    SessionManager::GetInstance().RegisterFactory(SessionType::CLIENT, []() {
        return new ClientSession();
        });


    // 3. ��Ŷ �ڵ鷯 �ʱ�ȭ
    InitGateHandlers();

    // 4. ��Ʈ��ũ ���ε� �� ����
    NetworkManager& network = NetworkManager::GetInstance();

    // 5. Accept ����
    network.InitFromConfig();

    // 6. ��Ŀ ������ ����
    iocp.Run();

    LOG("IOCP ���� ����\n");

    while (true)
    {
        Sleep(1000);
        // todo : Ŀ�ǵ�, �̺�Ʈ Ż�� 
    }

    // 7. ���� ó��
    iocp.ShutDown();
    WSACleanup();

    return 0;
}
