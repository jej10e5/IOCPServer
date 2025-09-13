#include "NetworkManager.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "IocpCore.h"
#include "GamePacketHandler.h"
#include "GameManager.h"
#include "DBManager.h"
#include "DBConfigReader.h"


void InitGameHandlers()
{
    // 패킷 핸들러 등록
    REGISTER_HANDLER(CM_ECHO, GamePacketHandler::Handle_Eco);
    REGISTER_HANDLER(CM_LOGIN, GamePacketHandler::Handle_Login);
    REGISTER_HANDLER(CM_CHAT, GamePacketHandler::Handle_Chat);
}

int main()
{
    // DB 세팅 로드
    CDBConfigReader dbcfg;
    if (!dbcfg.Load())
    {
        std::wcerr << L"[ERROR] db.ini 로딩 실패\n";
        return -1;
    }
    const DBConfig& db = dbcfg.DB();
    // DB 초기화 및 연결
    if (!DBManager::Instance().Initialize(db.connStr, db.poolSize, db.healthSec))
    {
        std::cerr << "[ERROR] DBManager 초기화 실패\n";
        return -2;
    }


    // 1. IOCP Core 초기화
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP 초기화 실패");
        return -1;
    }


    //2. SessionManager에 Session 생성 로직 주입
    SessionManager::GetInstance().RegisterFactory(SessionType::CLIENT, []() {
        return new ClientSession();
        });

    // 3. 패킷 핸들러 초기화
    InitGameHandlers();

   
    // 4. 네트워크 바인딩 및 리슨
    NetworkManager& network = NetworkManager::GetInstance();
    network.InitFromConfig();

    // 5. 워커 스레드 가동
    iocp.Run();

    LOG("IOCP 연결 시작\n");

    g_GameManager.StartLoop(std::chrono::milliseconds(50));

    for (string line; getline(cin, line); )
    {
        if (line == "quit") 
            break;
    }

    g_GameManager.StopLoop();

    // 7. 종료 처리
    iocp.ShutDown();
    WSACleanup();
    DBManager::Instance().Finalize();


    return 0;
}
