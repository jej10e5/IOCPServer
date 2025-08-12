#pragma once
#include "../ServerCommon/pch.h"
#include "Session.h"
class ServerSession : public Session
{
public:
	ServerSession()
	{
		Init();
	};

	void OnRecvCompleted(IocpContext* _pContext, DWORD _dwRecvLen) override;
};

