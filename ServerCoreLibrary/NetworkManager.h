#pragma once
#include "pch.h"
#include "Listener.h"
#include <functional>
#include "ConfigReader.h"
#include "SessionType.h"
class Session;
class ServerSession;
class NetworkManager:public Singleton<NetworkManager>
{
	friend class Singleton<NetworkManager>;
public:
	using SessionFactory = std::function<Session* ()>;

	void Init(UINT16 _uiPort, SessionType _eType);
	void AcceptListener(SessionType _eType);

	void InitFromConfig();

	

private:
	struct ListnerInfo
	{
		Listener listener;
		SessionType sessionType;
	};

	vector<ListnerInfo> m_Listeners;
};