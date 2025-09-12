#pragma once
#include "../ServerCommon/pch.h"
class CUnitPC
{
public:
	CUnitPC()
	{
		Init();
	};
	~CUnitPC() {};

	void Init();
	UINT64 GetSessionId() { return m_ui64SessionId; };
	void SetSessionId(UINT64 _uiID) { m_ui64SessionId = _uiID;};
	void SetName(const char* _pName);
	void GetName(char* _pName);
	INT64 GetUnique() { return m_i64Unique; };
	void SetUnique(INT64 _i64Unique) { m_i64Unique = _i64Unique; };


private:
	UINT64 m_ui64SessionId;
	INT64 m_i64Unique;
	char m_cName[MAX_ID_LENGTH];
};

