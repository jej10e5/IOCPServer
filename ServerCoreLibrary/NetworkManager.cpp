#include "pch.h"
#include "NetworkManager.h"
#include "Session.h"
#include "SessionManager.h"
void NetworkManager::Init(UINT16 _uiPort, SessionType _eType)
{
    ListnerInfo info;
    info.sessionType = _eType;

    if (!info.listener.Init(_uiPort))
    {
        ERROR_LOG("Failed to initialize listener on port: " << _uiPort);
        return;
    }

    // �����ʴ� �� ���� ���Ϳ� �ִ´�
    m_Listeners.push_back(std::move(info));

    // push_back ���Ŀ� back()���� ������ ��´�
    auto& listener = m_Listeners.back().listener;

    // ���� �����ʿ� ���� Accept�� ���� �� ���Խ�
    constexpr int ACCEPT_BACKLOG = 64; // �ʿ�� 128~256���� �÷��� OK
    for (int i = 0; i < ACCEPT_BACKLOG; ++i)
    {
        listener.PostAccept(_eType);
    }

    LOG("Listen started on port " << _uiPort << ", pre-posted accepts = " << ACCEPT_BACKLOG);

}

void NetworkManager::AcceptListener(SessionType _eType)
{
	for (auto& l : m_Listeners)
    {
        if (l.sessionType == _eType)
        {
			l.listener.PostAccept(_eType);
		}
	}
}

void NetworkManager::InitFromConfig()
{
    auto& cfg = ConfigReader::GetInstance();

    for (auto port : cfg.GetClientPorts())
        Init(static_cast<UINT16>(port), SessionType::CLIENT);

    for (auto port : cfg.GetGamePorts())
        Init(static_cast<UINT16>(port), SessionType::GAME);

    for (auto port : cfg.GetGatePorts())
        Init(static_cast<UINT16>(port), SessionType::GATE);
    
    //for (auto port : cfg.GetLoginPorts())
    //    Init(static_cast<UINT16>(port), SessionType::LOGIN);

}

