#include "pch.h"
#include "RingBuffer.h"


bool RingBuffer::Write(const char* _pData, INT32 _i32Len)
{
	// 빈 공간 있는지 체크
	if (_i32Len <= 0 || _i32Len > GetFreeSize())
		return false;

	// 한번 작성할 수 있는 크기
	INT32 i32FirstWritable = m_BufferSize - m_WritePos;
	if (i32FirstWritable >= _i32Len)
	{
		memcpy(&m_pBuffer[m_WritePos], _pData, _i32Len);
	}
	else
	{
		// 두개로 나눠서 카피
		memcpy(&m_pBuffer[m_WritePos], _pData, i32FirstWritable);
		memcpy(&m_pBuffer[0], _pData + i32FirstWritable, (_i32Len - i32FirstWritable));

	}

	// 다음 위치 잡아주기
	m_WritePos = (m_WritePos + _i32Len) % m_BufferSize;

	return true;
}

bool RingBuffer::Read(char*& _pOutData, INT32& _i32Len)
{
	// 저장된 길이가 패킷 길이보다 큰지 체크
	if (GetStoredSize() < _i32Len)
		return false;

	if (m_ReadPos < m_WritePos)
	{
		memcpy(_pOutData, &m_pBuffer[m_ReadPos], _i32Len);
	}
	else
	{
		// 한번 읽을 수 있는 크기
		INT32 i32FirstReadable = m_BufferSize - m_ReadPos;
		// 두개로 나눠서 카피
		memcpy(_pOutData, &m_pBuffer[m_ReadPos], i32FirstReadable);
		memcpy(_pOutData + i32FirstReadable, &m_pBuffer[0], _i32Len - i32FirstReadable);
	}

	m_ReadPos = (m_ReadPos + _i32Len) % m_BufferSize;

	return true;
}

bool RingBuffer::Peek(char* _pOutData, INT32 _i32Len)
{
	// 패킷 조립을 위해서 미리 보는게 필요한데 그 기능을 위함
	// Read는 포인터를 움직이기 때문에 보는것만 하기에는 위험함.

	// 일단 길이 검증부터
	if (_i32Len <= 0 || _i32Len > GetStoredSize())
		return false;

	// 읽는 부분부터 끝 위치를 확인하고
	// 한번에 읽을 수 있는지 체크
	INT32 i32FirstPeekable = m_BufferSize - m_ReadPos;
	if (i32FirstPeekable >= _i32Len)
	{
		memcpy(_pOutData, &m_pBuffer[m_ReadPos], _i32Len);
	}
	else
	{
		memcpy(_pOutData, &m_pBuffer[m_ReadPos], i32FirstPeekable);
		memcpy(_pOutData+i32FirstPeekable, &m_pBuffer[0], _i32Len - i32FirstPeekable);

	}

	return true;
}


bool RingBuffer::Remove(INT32 _i32Len)
{
	if (_i32Len == 0 || GetStoredSize() < _i32Len)
		return false;

	// peek하고 나서 한 만큼 옮겨주기
	m_ReadPos = (m_ReadPos + _i32Len) % m_BufferSize;

	return true;
}

INT32 RingBuffer::GetStoredSize() const
{
	if (m_WritePos >= m_ReadPos)
		return m_WritePos - m_ReadPos;
	else
		return m_BufferSize - m_ReadPos + m_WritePos;
}

INT32 RingBuffer::GetFreeSize() const
{
	return m_BufferSize - GetStoredSize() - 1;
}

void RingBuffer::Clear()
{
	// 링버퍼 내부 상태 초기화 (데이터는 보존됨)
	m_ReadPos = 0;
	m_WritePos = 0;
}
