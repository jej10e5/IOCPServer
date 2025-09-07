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
	// ���� ���� �Լ��� ���� -> ��� �޴� �ʿ��� ó���ϴ� �κ��� �޶���
	virtual void OnRecvCompleted(IocpContext* _pContext ,DWORD _dwRecvLen)=0;
	void OnSendCompleted(IocpContext* _pContext, DWORD _dwSendLen);
	void OnAcceptCompleted(IocpContext* _pContext);
	void Disconnect();
	void SetAcceptedSocket(SOCKET _socket) { m_AcceptSocket = _socket; }
	void SetListenerSocket(SOCKET _socket) { m_ListenSocket = _socket; }

	// ��Ŷ ������
	void SendPacket(const char* _pData, INT32 _i32Len) ;

private: 
	bool Send(const char* _pData, INT32 _i32Len); // SendPacket���ο����� ȣ���ϵ��� privateó��
	

public:
	SOCKET m_AcceptSocket;
	SOCKET m_ListenSocket;
	RingBuffer m_RecvBuffer;

private:
	SessionType m_eSessionType; // ���� Ÿ�� (Ŭ���̾�Ʈ, ����Ʈ, �α���, ���� ��)

	struct SendItem {
		std::shared_ptr<std::vector<char>> buf; // ���� ���� ������
		size_t off = 0;                         // ������� ���� ������
	};
	std::queue<SendItem> m_SendQueue; // ���� ��⿭
	std::mutex m_SendLock;
	bool m_bIsSending = false; // ���� ���� �� ����
};


