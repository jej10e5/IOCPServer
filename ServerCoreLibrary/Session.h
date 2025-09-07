#pragma once
#include "pch.h"
#include "RingBuffer.h"
#include "SessionType.h"
#include <memory>

struct IocpContext;

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
	void SetAcceptedSocket(SOCKET _socket) { m_AcceptSocket = _socket; }
	void SetListenerSocket(SOCKET _socket) { m_ListenSocket = _socket; }

	// 패킷 구현부
	void SendPacket(const char* _pData, INT32 _i32Len) ;

private: 
	bool Send(const char* _pData, INT32 _i32Len); // SendPacket내부에서만 호출하도록 private처리
	

public:
	SOCKET m_AcceptSocket;
	SOCKET m_ListenSocket;
	RingBuffer m_RecvBuffer;

private:
	SessionType m_eSessionType; // 세션 타입 (클라이언트, 게이트, 로그인, 게임 등)

	struct SendItem {
		std::shared_ptr<std::vector<char>> buf; // 실제 보낼 데이터
		size_t off = 0;                         // 현재까지 보낸 오프셋
	};
	std::queue<SendItem> m_SendQueue; // 전송 대기열
	std::mutex m_SendLock;
	bool m_bIsSending = false; // 현재 전송 중 여부
};


