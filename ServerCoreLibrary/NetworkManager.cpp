#include "pch.h"
#include "NetworkManager.h"

void NetworkManager::Init(UINT16 _uiPort, SessionFactory _factory)
{
	m_Listener.Init(_uiPort, _factory);
	m_Listener.PostAccept();
}

SOCKET NetworkManager::GetListenerSocket()
{

	return m_Listener.GetListenSocket();
}

void NetworkManager::AcceptListener()
{
	m_Listener.PostAccept();
}
