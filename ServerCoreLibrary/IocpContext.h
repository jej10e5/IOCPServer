#pragma once
#include "pch.h"
class Session;

// 이벤트의 용도 구분하기 위함
enum class IocpOperation : UINT8
{
    ACCEPT,     // 연결 수락
    RECV,       // 데이터 수신
    SEND        // 데이터 송신
};

struct IocpContext : OVERLAPPED
{
    IocpOperation eOperation;       // 어떤 작업 종류인지 구분
    Session* pSession;              // 어떤 세션에서 발생한 작업인지
    WSABUF tWsaBuf;                 // 수신/송신 버퍼
    char cBuffer[MAX_BUFFER_SIZE]; // 수신/송신 데이터 버퍼
};

