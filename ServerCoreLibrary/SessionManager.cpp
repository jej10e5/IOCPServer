#include "pch.h"
#include "SessionManager.h"
#include "Session.h"
class Session;

void SessionManager::RegistFactory(SessionType _eType, SessionFactory _factory, INT32 _i32PoolSize)
{
	// 일단 락 걸고
	std::lock_guard<std::mutex> guard(m_SessionLock);

	// 팩토리 설정
	m_Factories[_eType] = _factory;
	
	// 세션 타입이 등록되어 있지 않으면 새로 생성
	if (m_SessionPool.find(_eType) == m_SessionPool.end())
	{
		for (int i = 0; i < _i32PoolSize; i++)
		{
			m_SessionPool[_eType].push(m_Factories[_eType]());
		}
	}
}


Session* SessionManager::GetEmptySession(SessionType _eType)
{
	std::lock_guard<std::mutex> guard(m_SessionLock);

	if (m_SessionPool.find(_eType) == m_SessionPool.end() || m_SessionPool[_eType].empty())
	{
		if (m_Factories.find(_eType) == m_Factories.end())
			return nullptr; // 해당 타입의 팩토리가 등록되어 있지 않으면 nullptr 반환

		// 세션 풀에 해당 타입이 없거나 비어있으면 새로 생성
		return m_Factories[_eType]();
	}
	
	// 세션 풀에서 빈 세션을 가져옴
	Session* pSession = m_SessionPool[_eType].top();
	m_SessionPool[_eType].pop();
	return pSession;
}

void SessionManager::Release(SessionType _eType, Session* _pSession)
{
	std::lock_guard<std::mutex> guard(m_SessionLock);

	_pSession->Init();
	m_SessionPool[_eType].push(_pSession);
}
