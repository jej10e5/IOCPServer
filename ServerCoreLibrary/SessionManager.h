#pragma once
#include "pch.h"
#include <functional>
class  Session;
class SessionManager:public Singleton<SessionManager>
{
	friend class Singleton<SessionManager>;
public:
	void Init(std::function<Session*()> fnCreate);
	
	Session* GetEmptySession();
	void Release(Session* _pSession);

private:
	std::stack<Session*> m_SessionPool;
	std::mutex m_SessionLock;
	std::function<Session* ()> m_fnCreateSession; // 技记 积己 窃荐
};

