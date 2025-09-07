#include "pch.h"
#include "ServerSession.h"
#include "IocpContext.h"
#include "PacketDispatcher.h"

void ServerSession::OnRecvCompleted(IocpContext* _pContext, DWORD _dwRecvLen)
{
	// 1. ���� ���� 0�̸� Ŭ���̾�Ʈ�� ������ ��
	if (_dwRecvLen == 0)
	{
		// TODO : ���� �ݱ�, ���� ���� �ʿ� - Disconnect�Լ����� ����
		Disconnect(); // ���� ����
		return;
	}

	// 2. ���ŵ� ������ ���̸�ŭ ����
	m_RecvBuffer.Write(_pContext->tWsaBuf.buf, _dwRecvLen);

	// 3. ��Ŷ �ϼ� ���� Ȯ��
	while (true)
	{
		// 3-0. ��� ������ Ȯ��
		if (m_RecvBuffer.GetStoredSize() < PACKET_HEADER_SIZE)
			break;

		// 3-1. �ּ����� ������ -> ��� Ȯ��
		PacketHeader header;
		if (!m_RecvBuffer.Peek(reinterpret_cast<char*>(&header), PACKET_HEADER_SIZE))
		{
			ERROR_LOG("��Ŷ ��� Peek ����");
			break;
		}

		// ��Ŷ ������ ���ϱ�
		UINT16 ui16PacketSize = header.size;
		// ������ Ȯ��
		if (ui16PacketSize < PACKET_HEADER_SIZE || ui16PacketSize > MAX_RECV_BUFFER_SIZE)
		{
			ERROR_LOG("������ ��Ŷ ������ : " << ui16PacketSize);
			Disconnect(); // ���� ����
			return;
		}

		if (m_RecvBuffer.GetStoredSize() < ui16PacketSize)
			break;

		// 3-2. ���ۿ��� ������
		char cPacketBuffer[MAX_RECV_BUFFER_SIZE];
		// ������ ������ ����
		char* pReadPtr = cPacketBuffer;
		INT32 i32Len = static_cast<INT32>(ui16PacketSize);
		if (!m_RecvBuffer.Read(pReadPtr, i32Len) || i32Len != ui16PacketSize)
		{
			ERROR_LOG("��Ŷ Read ����");
			Disconnect(); // ���� ����
			return;
		}

		// ��Ŷ ó��
		PacketDispatcher& dispatcher = PacketDispatcher::GetInstance();
		dispatcher.Dispatch(this, pReadPtr, ui16PacketSize);

	}

	// 3. �ٽ� Recv() ȣ��
	Recv();

}
