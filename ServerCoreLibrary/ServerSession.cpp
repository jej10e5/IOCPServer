#include "pch.h"
#include "ServerSession.h"
#include "IocpContext.h"
#include "PacketDispatcher.h"

void ServerSession::OnRecvCompleted(IocpContext* _pContext, DWORD _dwRecvLen)
{
	// 1. 수신 길이 0이면 클라이언트가 종료한 것
	if (_dwRecvLen == 0)
	{
		// TODO : 소켓 닫기, 세션 해제 필요 - Disconnect함수에서 구현
		Disconnect(); // 연결 종료
		return;
	}

	// 2. 수신된 데이터 길이만큼 저장
	m_RecvBuffer.Write(_pContext->tWsaBuf.buf, _dwRecvLen);

	// 3. 패킷 완성 여부 확인
	while (true)
	{
		// 3-0. 헤더 사이즈 확인
		if (m_RecvBuffer.GetStoredSize() < PACKET_HEADER_SIZE)
			break;

		// 3-1. 최소한의 데이터 -> 헤더 확인
		PacketHeader header;
		if (!m_RecvBuffer.Peek(reinterpret_cast<char*>(&header), PACKET_HEADER_SIZE))
		{
			ERROR_LOG("패킷 헤더 Peek 실패");
			break;
		}

		// 패킷 사이즈 구하기
		UINT16 ui16PacketSize = header.size;
		// 사이즈 확인
		if (ui16PacketSize < PACKET_HEADER_SIZE || ui16PacketSize > MAX_RECV_BUFFER_SIZE)
		{
			ERROR_LOG("비정상 패킷 사이즈 : " << ui16PacketSize);
			Disconnect(); // 연결 종료
			return;
		}

		if (m_RecvBuffer.GetStoredSize() < ui16PacketSize)
			break;

		// 3-2. 버퍼에서 꺼내기
		char cPacketBuffer[MAX_RECV_BUFFER_SIZE];
		// 참조값 전달을 위함
		char* pReadPtr = cPacketBuffer;
		INT32 i32Len = static_cast<INT32>(ui16PacketSize);
		if (!m_RecvBuffer.Read(pReadPtr, i32Len) || i32Len != ui16PacketSize)
		{
			ERROR_LOG("패킷 Read 실패");
			Disconnect(); // 연결 종료
			return;
		}

		// 패킷 처리
		PacketDispatcher& dispatcher = PacketDispatcher::GetInstance();
		dispatcher.Dispatch(this, pReadPtr, ui16PacketSize);

	}

	// 3. 다시 Recv() 호출
	Recv();

}
