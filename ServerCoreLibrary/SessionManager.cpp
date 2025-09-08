#include "pch.h"
#include "SessionManager.h"
#include "Session.h"


SessionManager g_SessionManager;

void SessionManager::RegisterFactory(SessionType _eType, SessionFactory _factory, INT32 _i32PoolSize)
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

	_pSession->Init(_eType);
	m_SessionPool[_eType].push(_pSession);
}

SessionType SessionManager::StringToSessionType(const std::wstring& typeStr)
{
	if (typeStr == L"Client")
		return SessionType::CLIENT;
	else if(typeStr == L"Game")
		return SessionType::GAME;
	else if (typeStr == L"GameDB")
		return SessionType::GAMEDB;
	return SessionType::NONE; // �⺻������ Client ��ȯ
}

UINT64 SessionManager::RegisterActive(Session* _pSession)
{
	const UINT64 t = m_ui64NextToken.fetch_add(1, std::memory_order_relaxed);
	std::lock_guard<std::mutex> g(m_SessionLock);
	m_ActiveSessions[t] = _pSession;
	return t;
}

void SessionManager::UnregisterActive(Session* _pSession)
{
	std::lock_guard<std::mutex> g(m_ActiveLock);
	for (auto iter = m_ActiveSessions.begin(); iter != m_ActiveSessions.end();iter++)
	{
		if (iter->second == _pSession)
		{
			m_ActiveSessions.erase(iter);
			break;
		}
	}
}

Session* SessionManager::FindByToken(UINT64 _ui64Token)
{
	std::lock_guard<std::mutex> g(m_ActiveLock);
	auto iter = m_ActiveSessions.find(_ui64Token);
	return (iter == m_ActiveSessions.end()) ? nullptr : iter->second;
}

void SessionManager::BroadCastActive(const char* _pSendMsg, UINT16 _ui16size)
{
	vector<Session*> snapshot;
	{
		std::lock_guard<std::mutex> g(m_ActiveLock);
		snapshot.reserve(m_ActiveSessions.size());
		for (auto& s : m_ActiveSessions)
			snapshot.push_back(s.second);

	}
	for (auto iter : snapshot)
	{
		iter->SendPacket(_pSendMsg, _ui16size);
	}
}
