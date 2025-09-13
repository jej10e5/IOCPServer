#pragma once
#include "pch.h"
#include "SessionType.h"
class Session;

// 이벤트의 용도 구분하기 위함
enum class IocpOperation : UINT8
{
    ACCEPT,     // 연결 수락
    RECV,       // 데이터 수신
    SEND        // 데이터 송신
};

struct IocpContext
{
    OVERLAPPED ov;
    IocpOperation eOperation;       // 어떤 작업 종류인지 구분
   // SessionType eSessionType;
   // Session* pSession;              // 어떤 세션에서 발생한 작업인지
   // WSABUF tWsaBuf;                 // 수신/송신 버퍼
   // char cBuffer[MAX_BUFFER_SIZE]; // 수신/송신 데이터 버퍼
};


struct AcceptCtx : IocpContext
{
    SOCKET accepted = INVALID_SOCKET;
    SessionType sessionType;
    char addrBuf[(sizeof(SOCKADDR_IN) + 16) * 2]; // 로컬/리모트 주소 버퍼
};

struct RecvCtx : IocpContext
{
    Session* pSession = nullptr;
    WSABUF tWsaBuf;
    char cBuffer[MAX_BUFFER_SIZE];
};

struct SendCtx : IocpContext
{
    Session* pSession = nullptr;
    WSABUF tWsaBuf;
};