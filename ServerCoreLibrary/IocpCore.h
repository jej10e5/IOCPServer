#pragma once
#include "pch.h"
#include "IocpContext.h"

/// <summary>
/// IOCP Core : �� Ŭ���������� Ŀ�ο��� �Ϸ��� I/O �۾��� GQCS(GetQuueuedCompletionStatus())�� �޾Ƽ� ������ ����/�۾� ó���ڿ��� �����ϴ� ����
/// </summary>

class IocpCore : public Singleton<IocpCore>
{
	friend class Singleton<IocpCore>;
public:
	// IOCP �ڵ� ����
	bool Initialize();

	// ������ IOCP�� ���
	bool RegisterSocket(SOCKET _sock, ULONG_PTR _key);
	
	// ��Ŀ ������ ���� �� ���� ����
	void Run();

	// ���� �� ����
	void ShutDown();

	// GQCS ȣ�� ����
	void WorkerLoop();

private:
	// ������, �Ҹ��� ���� ���صθ� public������ �����Ϸ��� �ڵ� �����ؼ�
	// �� ���ٵ� ������ private���� ������
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
	HANDLE m_iocpHandle;	// iocp ť ��ü�� ����Ű�� �ڵ� - Ŀ�� I/O �۾� �Ϸ� �� �� �ڵ鿡 �˸��� ����
	vector<thread> m_workerThreads;  // IOCP ť�� �����ϸ鼭 �۾��� ����� ó���ϴ� Worker Thread Pool
	atomic<bool> m_isRunning; // ���� ������ ��� �ִ��� ���θ� ǥ��
	
};


