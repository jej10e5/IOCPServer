#pragma once
#include "pch.h"
#include "Session.h"
class ServerSession : public Session
{
public:
	ServerSession()
	{
		Init(SessionType::GAME);
	};

	void OnRecvCompleted(RecvCtx* _pContext, DWORD _dwRecvLen) override;
};

