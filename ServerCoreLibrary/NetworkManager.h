#pragma once
#include "pch.h"
#include "Listener.h"
#include <functional>
class Session;
class NetworkManager:public Singleton<NetworkManager>
{
	friend class Singleton<NetworkManager>;
public:
	using SessionFactory = std::function<Session* ()>;

	void Init(UINT16 _uiPort, SessionType _eType);
	SOCKET GetListenerSocket();
	void AcceptListener(SessionType _eType);
private:
	Listener m_Listener;
};