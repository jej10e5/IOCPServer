#include "pch.h"
#include "ConfigReader.h"


bool ConfigReader::LoadConfig(const std::wstring& _iniFilePath)
{
	m_IniFilePath = _iniFilePath;
	ParsePorts();
	return true;
}

INT32 ConfigReader::GetInt(const std::wstring& _section, const std::wstring& _key, INT32 _defaultValue)
{
	// GetPrivateProfileIntW 함수는 INI 파일에서 정수 값을 읽어오는 함수입니다.
	return GetPrivateProfileIntW(_section.c_str(), _key.c_str(), _defaultValue, m_IniFilePath.c_str());
}

std::wstring ConfigReader::GetString(const std::wstring& _section, const std::wstring& _key, const std::wstring& _defaultValue)
{
	// GetPrivateProfileStringW 함수는 INI 파일에서 문자열 값을 읽어오는 함수입니다.
	wchar_t buffer[256];
	GetPrivateProfileStringW(_section.c_str(), _key.c_str(), _defaultValue.c_str(), buffer, sizeof(buffer) / sizeof(wchar_t), m_IniFilePath.c_str());
	return std::wstring(buffer); // INI 파일에서 읽은 값을 반환
}

void ConfigReader::ParsePorts()
{
    wchar_t buffer[4096];
    DWORD size = GetPrivateProfileStringW(L"NetworkPort", NULL, L"", buffer, sizeof(buffer) / sizeof(wchar_t), m_IniFilePath.c_str());
    const wchar_t* key = buffer;

    while (*key)
    {
        INT32 port = GetInt(L"NetworkPort", key, 0);
        if (wcsncmp(key, L"Client", 6) == 0)
            m_ClientPorts.insert(port);
        else if (wcsncmp(key, L"Game", 4) == 0)
            m_GamePorts.insert(port);
        else if (wcsncmp(key, L"Gate", 4) == 0)
            m_GatePorts.insert(port);
        else if (wcsncmp(key, L"Login", 5) == 0)
            m_LoginPorts.insert(port);

        key += wcslen(key) + 1;
    }
}

