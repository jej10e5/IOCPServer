#include "pch.h"
#include "NetworkManager.h"

void NetworkManager::Init(UINT16 _uiPort, SessionType _eType)
{
	m_Listener.Init(_uiPort);
	m_Listener.PostAccept(_eType);
}

SOCKET NetworkManager::GetListenerSocket()
{

	return m_Listener.GetListenSocket();
}

void NetworkManager::AcceptListener(SessionType _eType)
{
	m_Listener.PostAccept(_eType);
}
