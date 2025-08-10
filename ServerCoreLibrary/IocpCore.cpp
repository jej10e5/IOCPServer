#include "pch.h"
#include "IocpCore.h"
#include "NetworkManager.h"

bool IocpCore::Initialize()
{
    m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    if (m_iocpHandle == NULL)
    {
        ERROR_LOG("CreateIoCompletionPort ���� : " << GetLastError());
        return false;
    }

    return true;
}

/// <summary>
/// ������ IOCP �ڵ鿡 ����ϴ� �Լ�
/// </summary>
/// <param name="_sock"></param>
/// <param name="_key"></param>
bool IocpCore::RegisterSocket(SOCKET _sock, ULONG_PTR _key)
{
    // CreateIoCompletionPort�� �̿��ؼ� ������ IOCP�� ����غ���.
    HANDLE result = CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(_sock), 
        m_iocpHandle, // iocp �ڵ�
        _key,         // i/o �Ϸ� �� �Ѿ�� Ű
        0);           // ó�� ����� �� ���õ�
    
    LOG("RegisterSocket ȣ�� - m_iocpHandle: " << m_iocpHandle << ", sock: " << _sock);


    // ������ ���� �����ϱ� ���� ó���� ������.
    if (result == NULL)
    {
        ERROR_LOG("CreateIoCompletionPort ��� ���� : " << GetLastError());
        return false;
    }

    return true;
}

void IocpCore::Run()
{
    m_isRunning = true;
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    INT32 i32ThreadCount = systemInfo.dwNumberOfProcessors * 2; // ���� ���

    for (int i = 0;i < i32ThreadCount;i++)
    {
        m_workerThreads.emplace_back([this]() {
            this->WorkerLoop();
            });
    }
}

void IocpCore::ShutDown()
{
	if (!m_isRunning.exchange(false))
    {
		// �̹� ���� ���� ���
		return;
	}

	for (size_t i = 0; i < m_workerThreads.size(); ++i)
    {
		::PostQueuedCompletionStatus(m_iocpHandle, 0, /*QUIT_KEY*/ 0xFFFFFFFF, nullptr); // ��Ŀ �����忡�� ���� ��ȣ ����
	}


	// ��� ��Ŀ �����尡 ����� ������ ���
	for (auto& thread : m_workerThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

    m_workerThreads.clear();

	// IOCP �ڵ� �ݱ�
	if (m_iocpHandle != NULL)
	{
		CloseHandle(m_iocpHandle);
		m_iocpHandle = NULL;
	}

	LOG("IOCP Core ����");
}

void IocpCore::WorkerLoop()
{
    while (m_isRunning)
    {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0; // ���� �ĺ��ڷ� ���
        OVERLAPPED* overlapped = nullptr; // I/O��û �� �ѱ� context ����ü ������

        //GQCS ȣ�� - IOCP ť���� �۾� ������ WinAPI�Լ�
        // IO �̺�Ʈ ����
        BOOL result = GetQueuedCompletionStatus(
            m_iocpHandle,
            &bytesTransferred,
            &completionKey,
            &overlapped,
            INFINITE
        );

        if (!m_isRunning) break; // ���� �÷��� Ȯ��

        if (result == FALSE || overlapped == nullptr)
        {
            // ������ �����ų� ���� �߻� �� ���� ���� ����
            continue;
        }

        // IocpContext�� ĳ���� (OVERLAPPED Ȯ�� ����)
        IocpContext* pContext = reinterpret_cast<IocpContext*>(overlapped);

        // �۾� ������ ���� �б�
        switch (pContext->eOperation)
        {
        case IocpOperation::RECV:
            pContext->pSession->OnRecvCompleted(pContext, bytesTransferred);
            break;
        case IocpOperation::SEND:
            pContext->pSession->OnSendCompleted(pContext, bytesTransferred);
            break;
        case IocpOperation::ACCEPT:
            pContext->pSession->OnAcceptCompleted(pContext);
            break;
        default:
            break;
        }

        delete pContext;
    }
}

