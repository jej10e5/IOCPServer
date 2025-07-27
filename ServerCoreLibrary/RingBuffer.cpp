#include "pch.h"
#include "RingBuffer.h"


bool RingBuffer::Write(const char* _pData, INT32 _i32Len)
{
	// �� ���� �ִ��� üũ
	if (_i32Len <= 0 || _i32Len > GetFreeSize())
		return false;

	// �ѹ� �ۼ��� �� �ִ� ũ��
	INT32 i32FirstWritable = m_BufferSize - m_WritePos;
	if (i32FirstWritable >= _i32Len)
	{
		memcpy(&m_pBuffer[m_WritePos], _pData, _i32Len);
	}
	else
	{
		// �ΰ��� ������ ī��
		memcpy(&m_pBuffer[m_WritePos], _pData, i32FirstWritable);
		memcpy(&m_pBuffer[0], _pData + i32FirstWritable, (_i32Len - i32FirstWritable));

	}

	// ���� ��ġ ����ֱ�
	m_WritePos = (m_WritePos + _i32Len) % m_BufferSize;

	return true;
}

bool RingBuffer::Read(char*& _pOutData, INT32& _i32Len)
{
	// ����� ���̰� ��Ŷ ���̺��� ū�� üũ
	if (GetStoredSize() < _i32Len)
		return false;

	if (m_ReadPos < m_WritePos)
	{
		memcpy(_pOutData, &m_pBuffer[m_ReadPos], _i32Len);
	}
	else
	{
		// �ѹ� ���� �� �ִ� ũ��
		INT32 i32FirstReadable = m_BufferSize - m_ReadPos;
		// �ΰ��� ������ ī��
		memcpy(_pOutData, &m_pBuffer[m_ReadPos], i32FirstReadable);
		memcpy(_pOutData + i32FirstReadable, &m_pBuffer[0], _i32Len - i32FirstReadable);
	}

	m_ReadPos = (m_ReadPos + _i32Len) % m_BufferSize;

	return true;
}

bool RingBuffer::Peek(char* _pOutData, INT32 _i32Len)
{
	// ��Ŷ ������ ���ؼ� �̸� ���°� �ʿ��ѵ� �� ����� ����
	// Read�� �����͸� �����̱� ������ ���°͸� �ϱ⿡�� ������.

	// �ϴ� ���� ��������
	if (_i32Len <= 0 || _i32Len > GetStoredSize())
		return false;

	// �д� �κк��� �� ��ġ�� Ȯ���ϰ�
	// �ѹ��� ���� �� �ִ��� üũ
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

	// peek�ϰ� ���� �� ��ŭ �Ű��ֱ�
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
	// ������ ���� ���� �ʱ�ȭ (�����ʹ� ������)
	m_ReadPos = 0;
	m_WritePos = 0;
}
