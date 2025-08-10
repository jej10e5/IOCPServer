#pragma once
#include "pch.h"
#include "IocpContext.h"

/// <summary>
/// IOCP Core : 이 클래스에서는 커널에서 완료한 I/O 작업을 GQCS(GetQuueuedCompletionStatus())로 받아서 적절한 세션/작업 처리자에게 전달하는 역할
/// </summary>

class IocpCore : public Singleton<IocpCore>
{
	friend class Singleton<IocpCore>;
public:
	// IOCP 핸들 생성
	bool Initialize();

	// 소켓을 IOCP에 등록
	bool RegisterSocket(SOCKET _sock, ULONG_PTR _key);
	
	// 워커 스레드 생성 후 루프 진입
	void Run();

	// 종료 시 정리
	void ShutDown();

	// GQCS 호출 루프
	void WorkerLoop();

private:
	// 생성자, 소멸자 선언 안해두면 public형으로 컴파일러가 자동 생성해서
	// 그 접근도 막고자 private으로 선언함
	IocpCore()
	{
		m_iocpHandle = NULL;
		m_isRunning = false;

	}

	~IocpCore() 
	{
		ShutDown();
		if (m_iocpHandle != NULL)
		{
			CloseHandle(m_iocpHandle);
			m_iocpHandle = NULL;
		}
	}


private:
	HANDLE m_iocpHandle;	// iocp 큐 자체를 가리키는 핸들 - 커널 I/O 작업 완료 시 이 핸들에 알림을 쌓음
	vector<thread> m_workerThreads;  // IOCP 큐를 감시하면서 작업이 생기면 처리하는 Worker Thread Pool
	atomic<bool> m_isRunning; // 서버 루프가 살아 있는지 여부를 표시
	
};


