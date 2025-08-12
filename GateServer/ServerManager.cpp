#include "ServerManager.h"

void ServerManager::AddServer(ServerSession* _pSession)
{
	if (_pSession)
	{
		m_ServerList.push_back(_pSession);
	}

}

ServerSession* ServerManager::GetNextServer()
{
	if (m_ServerList.empty())
	{
		return nullptr;
	}

	ServerSession* pSession = m_ServerList[m_NextIndex];
	m_NextIndex = (m_NextIndex + 1) % m_ServerList.size();
	return pSession;
}
