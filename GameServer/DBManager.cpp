#include "DBManager.h"
#include <iostream>

DBManager& DBManager::Instance() 
{
    static DBManager s;
    return s;
}

bool DBManager::Initialize(const std::wstring& _connStr, INT32 _i32poolSize, INT32 _i32healthSec) 
{
    try {
        m_env = std::make_unique<db::Env>();
        m_pool = std::make_unique<db::ConnPool>(*m_env, _i32poolSize, _connStr);
        m_healthSec = _i32healthSec > 0 ? _i32healthSec : 10;
        m_running = true;

        // ��Ŀ ������ Ǯ ���� (Ǯ ������� �����ϰ�)
        for (int i = 0; i < _i32poolSize; ++i) 
        {
            m_workers.emplace_back([this, i] { WorkerProc(i); });
        }
        // �ｺüũ ������
        m_healthThread = std::thread([this] { HealthProc(); });
        return true;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "[DB] Initialize failed: " << e.what() << "\n";
        return false;
    }
}

void DBManager::Finalize() 
{
    m_running = false;
    mq_cv.notify_all();

    for (auto& t : m_workers) 
    {
        if (t.joinable()) t.join();
    }

    if (m_healthThread.joinable()) 
        m_healthThread.join();

    m_pool.reset();
    m_env.reset();
}

void DBManager::Push(DbTask&& _task) 
{
    {
        std::lock_guard<std::mutex> lock(mq_mtx);
        mq.push(std::move(_task));
    }
    mq_cv.notify_one();
}

// Worker: ť���� ���� Conn �Ӵ� �� ���� �� ��ȯ
void DBManager::WorkerProc(int /*_workerIdx*/) 
{
    while (m_running) 
    {
        DbTask task;
        {
            std::unique_lock<std::mutex> lock(mq_mtx);
            mq_cv.wait(lock, [&] { return !m_running || !mq.empty(); });
            if (!m_running && mq.empty()) break;
            task = std::move(mq.front());
            mq.pop();
        }

        try 
        {
            auto lease = m_pool->Acquire();
            db::Conn& c = lease.get();
            task.fn(c); // ���⼭ SP ȣ�� �� ���� DB �۾� ����
        }
        catch (const std::exception& e) 
        {
            std::cerr << "[DB] Worker exception: " << e.what() << "\n";
        }
    }
}

// �ֱ������� SELECT 1 (���� ��ȿ�� Ȯ��)
void DBManager::HealthProc() 
{
    while (m_running) 
    {
        try 
        {
            auto lease = m_pool->Acquire();
            db::Stmt s(lease.get());
            s.ExecDirect(L"SELECT 1");
            // �ʿ��ϸ� ��� ���ε�/Fetch ���� ����(���� üũ��)
        }
        catch (...) 
        {
            std::cerr << "[DB] Health check failed\n";
        }

        for (int i = 0; i < m_healthSec && m_running; ++i)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


void DBManager::Enqueue_CheckAccount(const char* id, const char* pw, std::function<void(INT64)> onDone)
{
    Push(DbTask{
        [id = std::move(id), pw = std::move(pw), cb = std::move(onDone)](db::Conn& c)
        {
            INT64 unique = -1;
            bool ok = Call_CheckToken(c, id, pw, unique);

            if (cb) 
                cb(unique);
        }
        });
}

bool DBManager::Call_CheckToken(db::Conn& conn, const char* id, const char* pw, INT64& outUnique)
{
    db::Stmt st(conn);
    if (!st.Prepare(L"{CALL dbo.gp_Check_Account(?, ?, ?)}"))
    {
        return false;
    }

    // char* �� std::wstring ��ȯ (UTF-8 ����, �ʿ�� CP_ACP�� ����)
    int lenId = MultiByteToWideChar(CP_UTF8, 0, id, -1, nullptr, 0);
    std::wstring wid(lenId, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, id, -1, wid.data(), lenId);

    int lenPw = MultiByteToWideChar(CP_UTF8, 0, pw, -1, nullptr, 0);
    std::wstring wpw(lenPw, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, pw, -1, wpw.data(), lenPw);

    if (!st.BindInWChar(1, wid.c_str(), SQL_NTS)) return false;
    if (!st.BindInWChar(2, wpw.c_str(), SQL_NTS)) return false;
    if (!st.BindOutBigInt(3, &outUnique)) return false;

    return st.Exec();

}

