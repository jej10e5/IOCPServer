// DBConfigReader.cpp
#include "DBConfigReader.h"
#include <Windows.h>
#include <filesystem>
#include <algorithm>

using namespace std;

namespace
{
    // 한 줄 구현 → 같은 줄에 중괄호
    static bool IsSpace(wchar_t c) { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; }
}

std::wstring DBConfigReader::Trim(std::wstring s)
{
    auto itL = std::find_if_not(s.begin(), s.end(), IsSpace);
    auto itR = std::find_if_not(s.rbegin(), s.rend(), IsSpace).base();
    if (itL >= itR)
    {
        return L"";
    }

    s = std::wstring(itL, itR);

    if (!s.empty() && (s.front() == L'"' || s.front() == L'\'')) { s.erase(s.begin()); }
    if (!s.empty() && (s.back() == L'"' || s.back() == L'\'')) { s.pop_back(); }

    return s;
}

std::wstring DBConfigReader::ReadString(const std::wstring& section,
    const std::wstring& key,
    const std::wstring& def,
    const std::wstring& path)
{
    wchar_t buf[4096] = {};
    DWORD n = ::GetPrivateProfileStringW(section.c_str(),
        key.c_str(),
        def.c_str(),
        buf,
        static_cast<DWORD>(std::size(buf)),
        path.c_str());

    std::wstring v(buf, buf + n);
    return Trim(v);
}

int DBConfigReader::ReadInt(const std::wstring& section,
    const std::wstring& key,
    int def,
    const std::wstring& path)
{
    return static_cast<int>(::GetPrivateProfileIntW(section.c_str(),
        key.c_str(),
        def,
        path.c_str()));
}

bool DBConfigReader::Load(const std::wstring& path)
{
    // 1) ini 경로 결정: 입력 path가 비었으면 실행 파일 폴더의 db.ini 사용
    if (!path.empty())
    {
        m_iniPath = path;
    }
    else
    {
        wchar_t buf[MAX_PATH] = {};
        DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
        std::filesystem::path exePath(buf, buf + n);
        m_iniPath = (exePath.parent_path() / L"db.ini").wstring();
    }

    // 2) 파일 존재 확인
    if (!std::filesystem::exists(m_iniPath))
    {
        return false;
    }

    // 3) 값 읽기
    m_db.connStr = ReadString(L"db", L"conn", L"", m_iniPath);
    m_db.poolSize = ReadInt(L"db", L"poolSize", 4, m_iniPath);
    m_db.healthSec = ReadInt(L"db", L"healthSec", 10, m_iniPath);
    m_db.queryTimeoutSec = ReadInt(L"db", L"queryTimeoutSec", 5, m_iniPath);

    // 4) 기본값/범위 보정
    ApplyDefaults();

    // 5) 최소 유효성 검사
    return Validate();
}

void DBConfigReader::ApplyDefaults()
{
    if (m_db.poolSize < 1) { m_db.poolSize = 1; }
    if (m_db.poolSize > 64) { m_db.poolSize = 64; }

    if (m_db.healthSec < 1) { m_db.healthSec = 5; }
    if (m_db.healthSec > 3600) { m_db.healthSec = 3600; }

    if (m_db.queryTimeoutSec < 1) { m_db.queryTimeoutSec = 1; }
    if (m_db.queryTimeoutSec > 120) { m_db.queryTimeoutSec = 120; }
}

bool DBConfigReader::Validate() const
{
    if (m_db.connStr.empty())
    {
        return false;
    }

    // 간단한 구성 요소 존재 체크(운영에선 더 엄격한 검증 가능)
    if (m_db.connStr.find(L"Driver=") == std::wstring::npos) { return false; }
    if (m_db.connStr.find(L"Server=") == std::wstring::npos) { return false; }

    return true;
}
