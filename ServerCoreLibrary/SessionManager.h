#pragma once
#include "pch.h"
class  Session;
class SessionManager:public Singleton<SessionManager>
{
	friend class Singleton<SessionManager>;
public:
	void Init();
	Session* GetEmptySession();
	void Release(Session* _pSession);

private:
	std::stack<Session*> m_SessionPool;
	std::mutex m_SessionLock;
};

