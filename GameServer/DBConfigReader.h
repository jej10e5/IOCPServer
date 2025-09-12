#pragma once
#include "../ServerCommon/pch.h"
#include "../ServerCommon/Singleton.h"
struct DBConfig 
{
    std::wstring connStr;     // ODBC ���� ���ڿ�
    int poolSize = 4;         // ���� Ǯ ũ��
    int healthSec = 10;       // �ｺ üũ �ֱ�(��)
    int queryTimeoutSec = 5;  // ����: ���� Ÿ�Ӿƿ�(��)
};

class DBConfigReader :Singleton<DBConfigReader>
{
public:
    // path�� ���� ���� ���� ��ġ�� db.ini�� ã�´�.
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
