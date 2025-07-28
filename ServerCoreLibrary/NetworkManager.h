#pragma once
#include "pch.h"
#include "Listener.h"

class NetworkManager:public Singleton<NetworkManager>
{
	friend class Singleton<NetworkManager>;
public:
	void Init(UINT16 _uiPort);
	SOCKET GetListenerSocket();
	void AcceptListener();
private:
	Listener m_Listener;
};