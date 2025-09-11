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
	UINT64 GetId() { return m_ui64Id; };
	void SetId(UINT64 _uiID) { m_ui64Id = _uiID;};
	void SetName(char* _pName);
	void GetName(char* _pName);


private:
	UINT64 m_ui64Id;
	char m_cName[MAX_NICKNAME_LENGTH];
};

