#pragma once
#include "pch.h"
#include "RingBuffer.h"
#include "SessionType.h"

struct IocpContext;
struct SendBuffer
{
	vector<char> data;
	INT32 len;

	SendBuffer(const char* pData, INT32 i32Len)
		: data(pData, pData + i32Len), len(i32Len) {}

};

class Session
{
public:
	Session() : m_RecvBuffer(MAX_RECV_BUFFER_SIZE)
	{
		Init(SessionType::NONE);
	}
	void Init(SessionType _eType);
	bool Recv();
	// 순수 가상 함수로 구현 -> 상속 받는 쪽에서 처리하는 부분이 달라짐
	virtual void OnRecvCompleted(IocpContext* _pContext ,DWORD _dwRecvLen)=0;
	void OnSendCompleted(IocpContext* _pContext, DWORD _dwSendLen);
	void OnAcceptCompleted(IocpContext* _pContext);
	void Disconnect();
	void SetListenSocket(SOCKET _socket) { m_Socket = _socket; }

	// 패킷 구현부
	void SendPacket(const char* _pData, INT32 _i32Len) ;

private: 
	bool Send(const char* _pData, INT32 _i32Len); // SendPacket내부에서만 호출하도록 private처리

public:
	SOCKET m_Socket;
	RingBuffer m_RecvBuffer;

private:
	SessionType m_eSessionType; // 세션 타입 (클라이언트, 게이트, 로그인, 게임 등)
	std::queue<std::vector<char>> m_SendQueue; // 전송 대기열
	std::mutex m_SendLock;
	bool m_bIsSending = false; // 현재 전송 중 여부
};


