#include "pch.h"
#include "ConfigReader.h"

ConfigReader::ConfigReader(const std::wstring& _iniFilePath)
	: m_IniFilePath(_iniFilePath)
{
	// 초기화 작업이 필요하다면 여기에 추가
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
