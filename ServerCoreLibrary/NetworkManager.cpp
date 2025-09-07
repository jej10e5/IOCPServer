#include "pch.h"
#include "NetworkManager.h"
#include "Session.h"
#include "SessionManager.h"
void NetworkManager::Init(UINT16 _uiPort, SessionType _eType)
{
	ListnerInfo info;
	if (!info.listener.Init(_uiPort))
    {
		ERROR_LOG("Failed to initialize listener on port: " + _uiPort);
		return;
	}

	info.sessionType = _eType;  
	info.listener.PostAccept(_eType);

	m_Listeners.push_back(std::move(info));
    //auto& listener = m_Listeners.back().listener;
    //constexpr int ACCEPT_BACKLOG = 64;
    //for (int i = 0;i < ACCEPT_BACKLOG;i++)
    //    listener.PostAccept(_eType);


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

