#pragma once
#include "../ServerCommon/pch.h"
#include <condition_variable>
#include "../DbCore/OdbcCore.h"
#include "DBTypes.h"
#include <array>
#include <cstdint>
#include "../ServerCommon/Singleton.h"
//DB �۾� ����
struct DbTask
{
	std::function<void(db::Conn&)> fn;
};

class DBManager : Singleton<DBManager>
{
public:
    static DBManager& Instance();

    bool Initialize(const std::wstring& _connStr, INT32 _i32poolSize, INT32 _i32healthSec);
    void Finalize();

    // �۾� ���� (���� ���� �����忡�� ȣ��)
    void Push(DbTask&& _task);

    // ���� API 
    void Enqueue_CheckAccount(const char* id, const char* pw, std::function<void(INT64)> onDone);

	static bool Call_CheckToken(db::Conn& conn, const char* id, const char* pw, INT64& outToken);


private:
    DBManager() = default;
    ~DBManager() = default;

    void WorkerProc(int _workerIdx);
    void HealthProc();

private:
    std::unique_ptr<db::Env>          m_env;
    std::unique_ptr<db::ConnPool>     m_pool;

    std::vector<std::thread>          m_workers;
    std::thread                       m_healthThread;

    std::mutex                        mq_mtx;
    std::condition_variable           mq_cv;
    std::queue<DbTask>                mq;

    std::atomic<bool>                 m_running{ false };
    INT32                               m_healthSec{ 10 };
};

