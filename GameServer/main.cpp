#include "NetworkManager.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "IocpCore.h"
#include "GamePacketHandler.h"
#include "GameManager.h"


void InitGateHandlers()
{
    // 패킷 핸들러 등록
    REGISTER_HANDLER(CM_ECHO, GamePacketHandler::Handle_Eco);
    REGISTER_HANDLER(CM_LOGIN, GamePacketHandler::Handle_Login);
    REGISTER_HANDLER(CM_CHAT, GamePacketHandler::Handle_Chat);
}

int main()
{
    // 1. IOCP Core 초기화
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP 초기화 실패");
        return -1;
    }

    //2. SessionManager에 Session 생성 로직 주입
    //게이트에서는 클라이언트 세션과 서버 세션을 관리
    SessionManager::GetInstance().RegisterFactory(SessionType::CLIENT, []() {
        return new ClientSession();
        });


    // 3. 패킷 핸들러 초기화
    InitGateHandlers();

    // 4. 네트워크 바인딩 및 리슨
    NetworkManager& network = NetworkManager::GetInstance();

    // 5. Accept 시작
    network.InitFromConfig();

    // 6. 워커 스레드 가동
    iocp.Run();

    LOG("IOCP 연결 시작\n");

    g_GameManager.StartLoop(std::chrono::milliseconds(50));

    for (std::string line; std::getline(std::cin, line); )
        if (line == "q") break;

    g_GameManager.StopLoop();

    // 7. 종료 처리
    iocp.ShutDown();
    WSACleanup();

    return 0;
}
