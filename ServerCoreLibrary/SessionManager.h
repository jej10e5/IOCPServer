#pragma once
#include "pch.h"
#include <functional>
class  Session;
enum class SessionType
{
	Client,
	Server,
};
class SessionManager:public Singleton<SessionManager>
{
	friend class Singleton<SessionManager>;

public:
	typedef std::function<Session* ()> SessionFactory;

	void RegistFactory(SessionType _eType, SessionFactory _factory, INT32 _i32PoolSize = 1);
	
	Session* GetEmptySession(SessionType _eType);
	void Release(SessionType _eType, Session* _pSession);

private:
	std::unordered_map<SessionType, std::stack<Session*>> m_SessionPool;
	std::unordered_map<SessionType, SessionFactory> m_Factories;
	std::mutex m_SessionLock;
};

