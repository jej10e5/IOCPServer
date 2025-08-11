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
    ConfigReader configReader(iniPath.c_str());   // �� �̰ɷ�!
    UINT16 port = static_cast<UINT16>(configReader.GetInt(L"Network", L"Port", 7777));

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
    network.Init(port);
    // 5. Accept ����
    network.AcceptListener();
   

    LOG("GateServer ���� ����");

    // 6. IOCP ���� ����
    iocp.Run();


    while (true)
    {
        Sleep(1000);
    }

    // 7. ���� ó��
	iocp.ShutDown();


    WSACleanup();

    return 0;
}
