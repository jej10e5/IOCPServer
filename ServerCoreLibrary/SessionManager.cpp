#include "pch.h"
#include "SessionManager.h"
#include "Session.h"
class Session;
void SessionManager::Init(std::function<Session* ()> fnCreate)
{
	m_fnCreateSession = fnCreate;
	for (int i = 0;i < MAX_SESSION_SIZE;i++)
	{
		m_SessionPool.emplace(m_fnCreateSession());
	}
}


Session* SessionManager::GetEmptySession()
{
	std::lock_guard<std::mutex> guard(m_SessionLock);

	if (m_SessionPool.empty())
	{
		m_SessionPool.emplace(m_fnCreateSession());
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
