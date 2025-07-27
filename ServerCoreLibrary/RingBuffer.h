#pragma once
#include "pch.h"

class RingBuffer
{
public:
	RingBuffer()
	{
		Clear();
	};
	~RingBuffer()
	{
		Clear();
	};

	RingBuffer(size_t _size){
		// 버퍼 사이즈 지정
		m_BufferSize = _size;
		m_pBuffer = std::make_unique<char[]>(m_BufferSize);
	};


	bool Write(const char* _pData, INT32 _i32Len);
	bool Read(char*& _pOutData, INT32& _i32Len);
	bool Peek(char* _pOutData, INT32 _i32Len);
	bool Remove(INT32 _i32Len);

	INT32 GetStoredSize() const;
	INT32 GetFreeSize() const;

	void Clear();

private:
	std::unique_ptr<char[]> m_pBuffer;
	size_t m_BufferSize;
	size_t m_ReadPos;
	size_t m_WritePos;
	bool m_bIsEmpty;
};

