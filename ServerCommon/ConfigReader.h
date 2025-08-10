#pragma once
#include <string>
class ConfigReader
{
public:
	ConfigReader(const std::wstring& _iniFilePath);

	INT32 GetInt(const std::wstring& _section, const std::wstring& _key, INT32 _defaultValue = 0);
	std::wstring GetString(const std::wstring& _section, const std::wstring& _key, const std::wstring& _defaultValue = L"");

private:
	std::wstring m_IniFilePath;
};

