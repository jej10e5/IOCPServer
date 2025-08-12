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
	bool Init(UINT16 _port, SessionFactory _factory);
	bool PostAccept();
	SOCKET GetListenSocket() const { return m_ListenSocket; }

private:
	// SOCKET : Windows Winsock API에서 정의된 정수형 핸들 (커널 내부의 소켓 구조체를 가리키는 정수 핸들)
	SOCKET m_ListenSocket = INVALID_SOCKET;
	LPFN_ACCEPTEX m_lpfnAcceptEx = nullptr;
	SessionFactory m_Factory;
};

