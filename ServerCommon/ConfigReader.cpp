#include "pch.h"
#include "ConfigReader.h"

ConfigReader::ConfigReader(const std::wstring& _iniFilePath)
	: m_IniFilePath(_iniFilePath)
{
	// �ʱ�ȭ �۾��� �ʿ��ϴٸ� ���⿡ �߰�
}

INT32 ConfigReader::GetInt(const std::wstring& _section, const std::wstring& _key, INT32 _defaultValue)
{
	// GetPrivateProfileIntW �Լ��� INI ���Ͽ��� ���� ���� �о���� �Լ��Դϴ�.
	return GetPrivateProfileIntW(_section.c_str(), _key.c_str(), _defaultValue, m_IniFilePath.c_str());
}

std::wstring ConfigReader::GetString(const std::wstring& _section, const std::wstring& _key, const std::wstring& _defaultValue)
{
	// GetPrivateProfileStringW �Լ��� INI ���Ͽ��� ���ڿ� ���� �о���� �Լ��Դϴ�.
	wchar_t buffer[256];
	GetPrivateProfileStringW(_section.c_str(), _key.c_str(), _defaultValue.c_str(), buffer, sizeof(buffer) / sizeof(wchar_t), m_IniFilePath.c_str());
	return std::wstring(buffer); // INI ���Ͽ��� ���� ���� ��ȯ
}
