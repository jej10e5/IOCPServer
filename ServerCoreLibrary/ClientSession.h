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

	void OnRecvCompleted(IocpContext* _pContext, DWORD _dwRecvLen) override;
};
