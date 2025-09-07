#pragma once
#include "pch.h"
#include "ServerSession.h"
class ServerManager
{
public:
	static ServerManager& GetInstance()
	{
		static ServerManager instance;
		return instance;
	}

	void AddServer(ServerSession* _pSession);

	ServerSession* GetNextServer();

private:
	std::vector<ServerSession*> m_ServerList;
	size_t m_NextIndex = 0;
};

