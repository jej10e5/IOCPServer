#include "pch.h"
#include "SessionManager.h"
#include "Session.h"
class Session;

void SessionManager::RegistFactory(SessionType _eType, SessionFactory _factory, INT32 _i32PoolSize)
{
	// �ϴ� �� �ɰ�
	std::lock_guard<std::mutex> guard(m_SessionLock);

	// ���丮 ����
	m_Factories[_eType] = _factory;
	
	// ���� Ÿ���� ��ϵǾ� ���� ������ ���� ����
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
			return nullptr; // �ش� Ÿ���� ���丮�� ��ϵǾ� ���� ������ nullptr ��ȯ

		// ���� Ǯ�� �ش� Ÿ���� ���ų� ��������� ���� ����
		return m_Factories[_eType]();
	}
	
	// ���� Ǯ���� �� ������ ������
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
