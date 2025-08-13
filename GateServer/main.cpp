#include "../ServerCommon/pch.h"
#include "../ServerCommon/ConfigReader.h"
#include "IocpCore.h"
#include "GatePacketHandler.h"
#include "SessionManager.h"
#include "NetworkManager.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include <filesystem>

static std::wstring GetExeDir()
{
    wchar_t buf[MAX_PATH];
    DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf, buf + n).parent_path().wstring();
}

void InitGateHandlers()
{
	// 패킷 핸들러 등록
	REGISTER_HANDLER(CM_ECHO, GatePacketHandler::Handle_Eco);
}

int main()
{

    std::wstring iniPath = GetExeDir() + L"\\Server.ini";
    ConfigReader configReader(iniPath.c_str());   // ← 이걸로!
    UINT16 clientport = static_cast<UINT16>(configReader.GetInt(L"Port", L"Client", 14001));
    UINT16 serverport = static_cast<UINT16>(configReader.GetInt(L"Port", L"GameServer", 15001));

    // 1. Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ERROR_LOG("WSAStartup 실패");
        return -1;
    }

    // 2. SessionManager에 Session 생성 로직 주입
    // 게이트에서는 클라이언트 세션과 서버 세션을 관리합니다.
    SessionManager::GetInstance().RegistFactory(SessionType::Client,[]() {
        return new ClientSession();
        });
    SessionManager::GetInstance().RegistFactory(SessionType::Server, []() {
        return new ServerSession();
        });

	// 2-1. 패킷 핸들러 초기화
    InitGateHandlers();

    // 3. IOCP Core 초기화
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP 초기화 실패");
        return -1;
    }

    // 4. 네트워크 바인딩 및 리슨
    NetworkManager& network = NetworkManager::GetInstance();
    // 5. Accept 시작
    network.Init(clientport);
    network.Init(serverport);

    LOG("GateServer 실행 시작");

    // 6. IOCP 루프 진입
    iocp.Run();


    while (true)
    {
        Sleep(1000);
    }

    // 7. 종료 처리
	iocp.ShutDown();


    WSACleanup();

    return 0;
}
