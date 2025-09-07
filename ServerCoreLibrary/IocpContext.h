#pragma once
#include "pch.h"
class Session;

// �̺�Ʈ�� �뵵 �����ϱ� ����
enum class IocpOperation : UINT8
{
    ACCEPT,     // ���� ����
    RECV,       // ������ ����
    SEND        // ������ �۽�
};

struct IocpContext : OVERLAPPED
{
    IocpOperation eOperation;       // � �۾� �������� ����
    Session* pSession;              // � ���ǿ��� �߻��� �۾�����
    WSABUF tWsaBuf;                 // ����/�۽� ����
    char cBuffer[MAX_BUFFER_SIZE]; // ����/�۽� ������ ����
};

