#include "ClientSession.h"
#include "IocpContext.h"
#include "PacketDispatcher.h"

void ClientSession::OnRecvCompleted(IocpContext* _pContext, DWORD _dwRecvLen)
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
		// 3-1. �ּ����� ������ -> ��� Ȯ��
		PacketHeader header;
		m_RecvBuffer.Peek(reinterpret_cast<char*>(&header), sizeof(PacketHeader));
		// ��Ŷ ������ ���ϱ�
		UINT16 ui16PacketSize = header.size;
		// ������ Ȯ��
		if (m_RecvBuffer.GetStoredSize() < ui16PacketSize)
			break;

		// 3-2. ���ۿ��� ������
		char cPacketBuffer[MAX_RECV_BUFFER_SIZE];
		memset(cPacketBuffer, 0x00, sizeof(cPacketBuffer));
		// ������ ������ ����
		char* pReadPtr = cPacketBuffer;
		INT32 i32Len = static_cast<INT32>(ui16PacketSize);
		if (!m_RecvBuffer.Read(pReadPtr, i32Len))
		{
			ERROR_LOG("��Ŷ Read ����");
			break;
		}

		// ��Ŷ ó��
		PacketDispatcher& dispactcher = PacketDispatcher::GetInstance();
		dispactcher.Dispatch(this, pReadPtr, ui16PacketSize);

	}

	// 3. �ٽ� Recv() ȣ��
	Recv();

}