#include "SessionManager.h"
#include "Session.h"
void SessionManager::Init()
{
	for (int i = 0;i < MAX_SESSION_SIZE;i++)
	{
		m_SessionPool.emplace(new Session());
	}
}

Session* SessionManager::GetEmptySession()
{
	std::lock_guard<std::mutex> guard(m_SessionLock);

	if (m_SessionPool.empty())
	{
		m_SessionPool.emplace(new Session());
	}

	Session* pTempSession = m_SessionPool.top();
	m_SessionPool.pop();

	return pTempSession;
}

void SessionManager::Release(Session* _pSession)
{
	std::lock_guard<std::mutex> guard(m_SessionLock);

	_pSession->Init();
	m_SessionPool.push(_pSession);
}
