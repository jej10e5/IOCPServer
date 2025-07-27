#include "pch.h"
#include "NetworkManager.h"

void NetworkManager::Init()
{
	m_Listener.Init(DEFAULT_PORT);
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
