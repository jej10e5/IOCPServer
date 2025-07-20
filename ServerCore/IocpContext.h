#pragma once
#include "pch.h"
#include "Session.h"

// �̺�Ʈ�� �뵵 �����ϱ� ����
enum class IocpOperation
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

