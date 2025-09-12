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

        // 워커 스레드 풀 생성 (풀 사이즈와 동일하게)
        for (int i = 0; i < _i32poolSize; ++i) 
        {
            m_workers.emplace_back([this, i] { WorkerProc(i); });
        }
        // 헬스체크 스레드
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

// Worker: 큐에서 꺼내 Conn 임대 → 실행 → 반환
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
            task.fn(c); // 여기서 SP 호출 등 실제 DB 작업 수행
        }
        catch (const std::exception& e) 
        {
            std::cerr << "[DB] Worker exception: " << e.what() << "\n";
        }
    }
}

// 주기적으로 SELECT 1 (연결 유효성 확인)
void DBManager::HealthProc() 
{
    while (m_running) 
    {
        try 
        {
            auto lease = m_pool->Acquire();
            db::Stmt s(lease.get());
            s.ExecDirect(L"SELECT 1");
            // 필요하면 결과 바인딩/Fetch 생략 가능(간단 체크용)
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

    // char* → std::wstring 변환 (UTF-8 기준, 필요시 CP_ACP로 조정)
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

