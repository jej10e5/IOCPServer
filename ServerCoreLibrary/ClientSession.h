#pragma once
#include "pch.h"
#include "Session.h"
class ClientSession : public Session
{
public:
	ClientSession()
	{
		Init(SessionType::CLIENT);
	};

	void OnRecvCompleted(RecvCtx* _pContext, DWORD _dwRecvLen) override;
};
