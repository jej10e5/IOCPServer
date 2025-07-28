#pragma once
#include "Session.h"
class ClientSession : public Session
{
public:
	ClientSession()
	{
		Init();
	};

	void OnRecvCompleted(IocpContext* _pContext, DWORD _dwRecvLen) override;
};
