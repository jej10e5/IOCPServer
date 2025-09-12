// OdbcCore.h  (DbCore 퍼블릭 헤더 - PCH/외부 라이브러리 의존 제거)
#pragma once

#include <Windows.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <stdexcept>
#include <iterator> // std::size

#pragma comment(lib, "odbc32.lib")

namespace db
{
    // ----------------------------- Diag -----------------------------
    struct Diag
    {
        static std::wstring Get(SQLSMALLINT handleType, SQLHANDLE handle)
        {
            std::wstring out;
            SQLWCHAR state[6]{};
            SQLWCHAR msg[512]{};
            SQLINTEGER native = 0;
            SQLSMALLINT len = 0;

            for (SQLSMALLINT i = 1;
                SQLGetDiagRecW(handleType, handle, i, state, &native, msg, (SQLSMALLINT)std::size(msg) - 1, &len) == SQL_SUCCESS;
                ++i)
            {
                out.append(L"["); out.append(state); out.append(L"] ");
                out.append(msg); out.append(L" ("); out.append(std::to_wstring(native)); out.append(L")\n");
            }
            return out;
        }

        static void Log(SQLSMALLINT handleType, SQLHANDLE handle, const wchar_t* where = L"")
        {
            auto text = Get(handleType, handle);
            if (!text.empty())
            {
                OutputDebugStringW(L"[ODBC] ");
                OutputDebugStringW(where);
                OutputDebugStringW(L"\n");
                OutputDebugStringW(text.c_str());
            }
        }
    };

    // ----------------------------- Env ------------------------------
    class Env
    {
    public:
        Env()
        {
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
            if (!SQL_SUCCEEDED(rc)) throw std::runtime_error("SQLAllocHandle ENV failed");
            rc = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_ENV, m_hEnv, L"SQLSetEnvAttr(ODBC3)");
                throw std::runtime_error("Set ODBC version failed");
            }
        }

        ~Env()
        {
            if (m_hEnv)
            {
                SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
                m_hEnv = SQL_NULL_HENV;
            }
        }

        SQLHENV handle() const { return m_hEnv; }

    private:
        SQLHENV m_hEnv{ SQL_NULL_HENV };
    };

    // ----------------------------- Conn -----------------------------
    class Conn
    {
    public:
        explicit Conn(SQLHENV hEnv)
        {
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &m_hDbc);
            if (!SQL_SUCCEEDED(rc)) throw std::runtime_error("SQLAllocHandle DBC failed");
            SQLSetConnectAttr(m_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
            SQLSetConnectAttr(m_hDbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)5, 0);
        }

        ~Conn()
        {
            Disconnect();
            if (m_hDbc)
            {
                SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
                m_hDbc = SQL_NULL_HDBC;
            }
        }

        bool Connect(const std::wstring& connStr)
        {
            Disconnect();
            SQLRETURN rc = SQLDriverConnectW(m_hDbc, NULL,
                (SQLWCHAR*)connStr.c_str(), SQL_NTS,
                nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_DBC, m_hDbc, L"SQLDriverConnectW");
                return false;
            }
            m_connected = true;
            return true;
        }

        void Disconnect()
        {
            if (m_connected)
            {
                SQLEndTran(SQL_HANDLE_DBC, m_hDbc, SQL_ROLLBACK);
                SQLDisconnect(m_hDbc);
                m_connected = false;
            }
        }

        class Tx
        {
        public:
            explicit Tx(Conn& c) : m_c(c)
            {
                SQLSetConnectAttr(m_c.m_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
                m_active = true;
            }

            ~Tx()
            {
                if (!m_active) return;
                SQLEndTran(SQL_HANDLE_DBC, m_c.m_hDbc, SQL_ROLLBACK);
                SQLSetConnectAttr(m_c.m_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            }

            void Commit()
            {
                SQLEndTran(SQL_HANDLE_DBC, m_c.m_hDbc, SQL_COMMIT);
                SQLSetConnectAttr(m_c.m_hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
                m_active = false;
            }

        private:
            Conn& m_c;
            bool  m_active{ false };
        };

        SQLHDBC handle() const { return m_hDbc; }
        bool    connected() const { return m_connected; }

    private:
        SQLHDBC m_hDbc{ SQL_NULL_HDBC };
        bool    m_connected{ false };
    };

    // ----------------------------- Stmt -----------------------------
    class Stmt
    {
    public:
        explicit Stmt(Conn& c) : m_conn(c)
        {
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, c.handle(), &m_hStmt);
            if (!SQL_SUCCEEDED(rc)) throw std::runtime_error("SQLAllocHandle STMT failed");
            SQLSetStmtAttr(m_hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)5, 0);
        }

        ~Stmt()
        {
            if (m_hStmt)
            {
                SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
                m_hStmt = SQL_NULL_HSTMT;
            }
        }

        bool Prepare(const std::wstring& call)
        {
            SQLRETURN rc = SQLPrepareW(m_hStmt, (SQLWCHAR*)call.c_str(), SQL_NTS);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"SQLPrepareW");
                return false;
            }
            return true;
        }

        bool Exec()
        {
            SQLRETURN rc = SQLExecute(m_hStmt);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"SQLExecute");
                return false;
            }
            return true;
        }

        bool ExecDirect(const std::wstring& sql)
        {
            SQLRETURN rc = SQLExecDirectW(m_hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"SQLExecDirectW");
                return false;
            }
            return true;
        }

        bool BindInWChar(SQLUSMALLINT idx, const wchar_t* str, SQLLEN ntLen)
        {
            SQLRETURN rc = SQLBindParameter(m_hStmt, idx, SQL_PARAM_INPUT,
                SQL_C_WCHAR, SQL_WVARCHAR, 0, 0,
                (SQLPOINTER)str, 0, (SQLLEN*)&ntLen);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"BindInWChar");
                return false;
            }
            return true;
        }

        bool BindInBinary(SQLUSMALLINT idx, const void* buf, SQLLEN len)
        {
            SQLRETURN rc = SQLBindParameter(m_hStmt, idx, SQL_PARAM_INPUT,
                SQL_C_BINARY, SQL_VARBINARY, len, 0,
                (SQLPOINTER)buf, len, &m_tmpInd);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"BindInBinary");
                return false;
            }
            return true;
        }

        bool BindOutBigInt(SQLUSMALLINT idx, long long* out)
        {
            SQLRETURN rc = SQLBindParameter(m_hStmt, idx, SQL_PARAM_OUTPUT,
                SQL_C_SBIGINT, SQL_BIGINT, 0, 0, out, 0, &m_tmpInd);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"BindOutBigInt");
                return false;
            }
            return true;
        }

        bool BindOutInt(SQLUSMALLINT idx, int* out)
        {
            SQLRETURN rc = SQLBindParameter(m_hStmt, idx, SQL_PARAM_OUTPUT,
                SQL_C_SLONG, SQL_INTEGER, 0, 0, out, 0, &m_tmpInd);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"BindOutInt");
                return false;
            }
            return true;
        }

        bool BindColInt(SQLUSMALLINT col, int* out)
        {
            SQLRETURN rc = SQLBindCol(m_hStmt, col, SQL_C_SLONG, out, 0, nullptr);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"BindColInt");
                return false;
            }
            return true;
        }

        bool BindColWChar(SQLUSMALLINT col, wchar_t* buf, SQLLEN bufChars)
        {
            SQLRETURN rc = SQLBindCol(m_hStmt, col, SQL_C_WCHAR, buf, bufChars * sizeof(wchar_t), &m_tmpInd);
            if (!SQL_SUCCEEDED(rc))
            {
                Diag::Log(SQL_HANDLE_STMT, m_hStmt, L"BindColWChar");
                return false;
            }
            return true;
        }

        SQLRETURN Fetch() { return SQLFetch(m_hStmt); }
        SQLHSTMT  handle() const { return m_hStmt; }

    private:
        Conn& m_conn;
        SQLHSTMT m_hStmt{ SQL_NULL_HSTMT };
        SQLLEN   m_tmpInd{ 0 };
    };

    // --------------------------- ConnPool ---------------------------
    class ConnPool
    {
    public:
        ConnPool(Env& env, size_t poolSize, const std::wstring& connStr) : m_env(env), m_connStr(connStr)
        {
            assert(poolSize > 0);
            m_pool.reserve(poolSize);
            for (size_t i = 0; i < poolSize; ++i)
            {
                auto c = std::make_unique<Conn>(m_env.handle());
                if (!c->Connect(m_connStr)) throw std::runtime_error("DB connect failed (pool init)");
                m_pool.push_back(std::move(c));
                m_free.push_back(i);
            }
        }

        ~ConnPool()
        {
            for (auto& c : m_pool) c->Disconnect();
        }

        class Lease
        {
        public:
            Lease(ConnPool& p, Conn* c, size_t idx) : m_pool(p), m_conn(c), m_idx(idx) {}
            ~Lease() { if (m_conn) m_pool.Release(m_idx); }
            Conn& get() { return *m_conn; }
            Conn* operator->() { return m_conn; }
        private:
            ConnPool& m_pool;
            Conn* m_conn;
            size_t    m_idx;
        };

        Lease Acquire()
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock, [&] { return !m_free.empty(); });
            size_t idx = m_free.back(); m_free.pop_back();
            return Lease(*this, m_pool[idx].get(), idx);
        }

    private:
        void Release(size_t idx)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_free.push_back(idx);
            m_cv.notify_one();
        }

        Env& m_env;
        std::wstring                       m_connStr;
        std::vector<std::unique_ptr<Conn>> m_pool;
        std::vector<size_t>                m_free;
        std::mutex                         m_mtx;
        std::condition_variable            m_cv;
    };

} // namespace db
