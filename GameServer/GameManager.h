#pragma once
#include "../ServerCommon/pch.h"

class CUnitPC;
class CGameManager
{
public:
	CGameManager()
	{
		Init();
	};
	~CGameManager() {};

	void Init();

	void Login(UINT64 _ui64Id, char* _pName);
	CUnitPC* FindUserById(UINT64 _ui64Id);
	CUnitPC* GetEmptyPC();
	void ReleasePC(CUnitPC* _pUser);


private:
	vector<CUnitPC*> m_Users;
	vector<CUnitPC*> m_EmptyUnits;

};

extern CGameManager g_GameManager;

