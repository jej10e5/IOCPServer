#pragma once
#include "../ServerCommon/pch.h"
#include "../ServerCommon/Singleton.h"
struct DBConfig 
{
    std::wstring connStr;     // ODBC 연결 문자열
    int poolSize = 4;         // 연결 풀 크기
    int healthSec = 10;       // 헬스 체크 주기(초)
    int queryTimeoutSec = 5;  // 선택: 쿼리 타임아웃(초)
};

class DBConfigReader :Singleton<DBConfigReader>
{
public:
    // path를 비우면 실행 파일 위치의 db.ini를 찾는다.
    bool Load(const std::wstring& path = L"");

    const DBConfig& DB() const { return m_db; }
    const std::wstring& IniPath() const { return m_iniPath; }

private:
    static std::wstring Trim(std::wstring s);
    static std::wstring ReadString(const std::wstring& section,
        const std::wstring& key,
        const std::wstring& def,
        const std::wstring& path);
    static int ReadInt(const std::wstring& section,
        const std::wstring& key,
        int def,
        const std::wstring& path);
    void ApplyDefaults();
    bool Validate() const;

private:
    std::wstring m_iniPath;
    DBConfig     m_db;
};
