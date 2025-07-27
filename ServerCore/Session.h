#pragma once
#include "pch.h"
#include "SessionManager.h"
#include "RingBuffer.h"

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
		Init();
	}
	void Init();
	bool Recv();
	virtual void OnRecvCompleted(IocpContext* _pContext ,DWORD _dwRecvLen)=0;
	void OnSendCompleted(IocpContext* _pContext, DWORD _dwSendLen);
	void OnAcceptCompleted(IocpContext* _pContext);
	void Disconnect();

	// 패킷 구현부
	void SendPacket(const char* _pData, INT32 _i32Len) ;


private: 
	bool Send(const char* _pData, INT32 _i32Len); // SendPacket내부에서만 호출하도록 private처리

public:
	SOCKET m_Socket;
	RingBuffer m_RecvBuffer;

private:
	std::queue<std::vector<char>> m_SendQueue; // 전송 대기열
	std::mutex m_SendLock;
	bool m_bIsSending = false; // 현재 전송 중 여부
};


