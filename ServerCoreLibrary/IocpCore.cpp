#include "pch.h"
#include "IocpCore.h"
#include "NetworkManager.h"

bool IocpCore::Initialize()
{
    // 1. Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ERROR_LOG("WSAStartup 실패");
        return false;
    }

    // 2. 완료 포트 생성
    m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    if (m_iocpHandle == NULL)
    {
        ERROR_LOG("CreateIoCompletionPort 실패 : " << GetLastError());
        return false;
    }

    return true;
}

/// <summary>
/// 소켓을 IOCP 핸들에 등록하는 함수
/// </summary>
/// <param name="_sock"></param>
/// <param name="_key"></param>
bool IocpCore::RegisterSocket(SOCKET _sock, ULONG_PTR _key)
{
    // CreateIoCompletionPort를 이용해서 소켓을 IOCP에 등록해보자.
    HANDLE result = CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(_sock), 
        m_iocpHandle, // iocp 핸들
        _key,         // i/o 완료 시 넘어올 키
        0);           // 처음 등록일 땐 무시됨
    
    LOG("RegisterSocket 호출 - m_iocpHandle: " << m_iocpHandle << ", sock: " << _sock);


    // 실패할 수도 있으니까 예외 처리도 해주자.
    if (result == NULL)
    {
        ERROR_LOG("CreateIoCompletionPort 등록 실패 : " << GetLastError());
        return false;
    }

    return true;
}

void IocpCore::Run()
{
    m_isRunning = true;
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    INT32 i32ThreadCount = systemInfo.dwNumberOfProcessors * 2; // 권장 사양

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
		// 이미 종료 중인 경우
		return;
	}

	for (size_t i = 0; i < m_workerThreads.size(); ++i)
    {
		::PostQueuedCompletionStatus(m_iocpHandle, 0, /*QUIT_KEY*/ 0xFFFFFFFF, nullptr); // 워커 스레드에게 종료 신호 전송
	}


	// 모든 워커 스레드가 종료될 때까지 대기
	for (auto& thread : m_workerThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

    m_workerThreads.clear();

	// IOCP 핸들 닫기
	if (m_iocpHandle != NULL)
	{
		CloseHandle(m_iocpHandle);
		m_iocpHandle = NULL;
	}

	LOG("IOCP Core 종료");
}

void IocpCore::WorkerLoop()
{
    while (m_isRunning)
    {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0; // 소켓 식별자로 사용
        OVERLAPPED* overlapped = nullptr; // I/O요청 시 넘긴 context 구조체 포인터

        //GQCS 호출 - IOCP 큐에서 작업 꺼내는 WinAPI함수
        // IO 이벤트 수신
        BOOL result = GetQueuedCompletionStatus(
            m_iocpHandle,
            &bytesTransferred,
            &completionKey,
            &overlapped,
            INFINITE
        );

        if (!m_isRunning) break; // 종료 플래그 확인

        if (overlapped == nullptr)
        {
            continue;
        }

        // IocpContext로 캐스팅 (OVERLAPPED 확장 구조)
        IocpContext* pContext = reinterpret_cast<IocpContext*>(overlapped);

        // 오류 완료 처리 : 세션 정리/메모리 누수 방지
        if (result == FALSE) 
        {
             DWORD err = GetLastError();
            // RECV/SEND 오류 → 세션 종료
            if (pContext->eOperation == IocpOperation::RECV || pContext->eOperation == IocpOperation::SEND) 
            {
                pContext->pSession->Disconnect();
                
            }
            // ACCEPT 오류 → 세션 반환 및 다음 Accept 재포스트는 Session/Listener 쪽 로직에 위임
            delete pContext;
            continue;
            
        }

        // 작업 종류에 따라 분기
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

