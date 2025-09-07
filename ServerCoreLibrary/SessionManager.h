#pragma once
#include "pch.h"
#include <functional>
#include "../ServerCommon/Singleton.h"
#include "SessionType.h"
class Session;
class SessionManager :public Singleton<SessionManager>
{
	friend class Singleton<SessionManager>;

public:
	typedef std::function<Session* ()> SessionFactory;

	void RegisterFactory(SessionType _eType, SessionFactory _factory, INT32 _i32PoolSize = 1);

	Session* GetEmptySession(SessionType _eType);
	void Release(SessionType _eType, Session* _pSession);

	SessionType StringToSessionType(const std::wstring& typeStr);

	UINT64 RegisterActive(Session* _pSession);
	void UnregisterActive(Session* _pSession);
	Session* FinByToken(UINT64 _ui64Token);


private:
	std::atomic<UINT64> m_ui64NextToken{ 1 };
	std::unordered_map<UINT64, Session*> m_ActiveSessios;
	std::mutex m_ActiveLock;

	std::unordered_map<SessionType, std::stack<Session*>> m_SessionPool;
	std::unordered_map<SessionType, SessionFactory> m_Factories;
	std::mutex m_SessionLock;
};

