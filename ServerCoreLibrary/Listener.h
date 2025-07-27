#pragma once
#include "pch.h"
class Listener
{
public:
	Listener() {};
	~Listener() {};

	bool Init(UINT16 _port);
	bool PostAccept();
	SOCKET GetListenSocket() const { return m_ListenSocket; }

private:
	// SOCKET : Windows Winsock API���� ���ǵ� ������ �ڵ� (Ŀ�� ������ ���� ����ü�� ����Ű�� ���� �ڵ�)
	SOCKET m_ListenSocket = INVALID_SOCKET;
	LPFN_ACCEPTEX m_lpfnAcceptEx = nullptr;
};

