#pragma once
#include "pch.h"
#include "Session.h"
#include <functional>
class Listener
{
public:
	Listener() {};
	~Listener() {};

	using SessionFactory = std::function<Session* ()>;
	bool Init(UINT16 _port);
	bool PostAccept(SessionType _eType);
	SOCKET GetListenSocket() const { return m_ListenSocket; }

private:
	// SOCKET : Windows Winsock API���� ���ǵ� ������ �ڵ� (Ŀ�� ������ ���� ����ü�� ����Ű�� ���� �ڵ�)
	SOCKET m_ListenSocket = INVALID_SOCKET;
	LPFN_ACCEPTEX m_lpfnAcceptEx = nullptr;
};

