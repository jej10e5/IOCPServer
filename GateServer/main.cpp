#include "../ServerCommon/pch.h"
#include "NetworkManager.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "ServerSession.h"
#include "GatePacketHandler.h"
#include "IocpCore.h"
#include <filesystem>

static std::wstring GetExeDir()
{
    wchar_t buf[MAX_PATH];
    DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf, buf + n).parent_path().wstring();
}

void InitGateHandlers()
{
	// ��Ŷ �ڵ鷯 ���
	REGISTER_HANDLER(CM_ECHO, GatePacketHandler::Handle_Eco);
}

int main()
{

    std::wstring iniPath = GetExeDir() + L"\\Server.ini";
	//ConfigReader::GetInstance().LoadConfig(iniPath.c_str());
   

    // 1. Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ERROR_LOG("WSAStartup ����");
        return -1;
    }

     //2. SessionManager�� Session ���� ���� ����
     //����Ʈ������ Ŭ���̾�Ʈ ���ǰ� ���� ������ �����մϴ�.
    SessionManager::GetInstance().RegistFactory(SessionType::CLIENT,[]() {
        return new ClientSession();
        });
    SessionManager::GetInstance().RegistFactory(SessionType::GAME, []() {
        return new ServerSession();
        });

	// 2-1. ��Ŷ �ڵ鷯 �ʱ�ȭ
    InitGateHandlers();

    // 3. IOCP Core �ʱ�ȭ
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.Initialize())
    {
        ERROR_LOG("IOCP �ʱ�ȭ ����");
        return -1;
    }

    // 4. ��Ʈ��ũ ���ε� �� ����
    NetworkManager& network = NetworkManager::GetInstance();
    // 5. Accept ����
	network.InitFromConfig();

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
