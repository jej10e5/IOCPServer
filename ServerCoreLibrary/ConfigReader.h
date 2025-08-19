#pragma once
#include <string>
#include <set>
#include "../ServerCommon/Singleton.h"
class ConfigReader : public Singleton<ConfigReader>
{
public:
	ConfigReader() { LoadConfig(); };
	bool LoadConfig();

	INT32 GetInt(const std::wstring& _section, const std::wstring& _key, INT32 _defaultValue = 0);
	std::wstring GetString(const std::wstring& _section, const std::wstring& _key, const std::wstring& _defaultValue = L"");
	
	const std::set<INT32>& GetClientPorts() const { return m_ClientPorts; }
	const std::set<INT32>& GetGamePorts() const { return m_GamePorts; }
	const std::set<INT32>& GetGatePorts() const { return m_GatePorts; }
	const std::set<INT32>& GetLoginPorts() const { return m_LoginPorts; }

private:
	void ParsePorts();

private:
	std::wstring m_IniFilePath;
	set<INT32> m_ClientPorts;
	set<INT32> m_GamePorts;
	set<INT32> m_GatePorts;
	set<INT32> m_LoginPorts;
};

