#include "../ServerCommon/pch.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "NetworkManager.h"
#include "IocpCore.h"
#include "../ServerCommon/ConfigReader.h"
#include <filesystem>

static std::wstring GetExeDir()
{
    wchar_t buf[MAX_PATH];
    DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf, buf + n).parent_path().wstring();
}

int main()
{

    std::wstring iniPath = GetExeDir() + L"\\Server.ini";
    ConfigReader configReader(iniPath.c_str());   // ← 이걸로!
    UINT16 port = static_cast<UINT16>(configReader.GetInt(L"Network", L"Port", 7777));

    // 1. Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ERROR_LOG("WSAStartup 실패");
        return -1;
    }

    // 2. SessionManager에 ClientSession 생성 로직 주입
    SessionManager::GetInstance().Init([]() {
        return new ClientSession();
        });

    // 3. IOCP Core 초기화
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP 초기화 실패");
        return -1;
    }

    // 4. 네트워크 바인딩 및 리슨
    NetworkManager& network = NetworkManager::GetInstance();
    network.Init(port);
    // 5. Accept 시작
    network.AcceptListener();
   

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
