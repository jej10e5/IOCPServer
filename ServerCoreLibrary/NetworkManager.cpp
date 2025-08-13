#include "pch.h"
#include "NetworkManager.h"

void NetworkManager::Init(UINT16 _uiPort)
{
	m_Listener.Init(_uiPort);
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
