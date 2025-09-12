#pragma once
#include "../ServerCommon/pch.h"

class CUnitPC;
class Session;
class CGameManager
{
public:
	CGameManager()
	{
		Init();
	};
	~CGameManager() {};

	void Init();
	bool StartLoop(std::chrono::milliseconds tick = std::chrono::milliseconds(50)); // 20TPS
	void StopLoop();
	void Login(Session* _pSession, const INT64 _ui64Unique, const char* _pName);
	void Logout(UINT64 _ui64Id);
	CUnitPC* FindUserById(UINT64 _ui64Id);
	CUnitPC* FindUserByUnique(INT64 _i64unique);
	CUnitPC* GetEmptyPC();
	void ReleasePC(CUnitPC* _pUser);


private:
	void RunLoop();

	vector<CUnitPC*> m_Users;
	vector<CUnitPC*> m_EmptyUnits;
	unordered_map<UINT64, CUnitPC*> m_SessionIdToPC;
	std::mutex m_UserMtx; 
	std::atomic<bool> m_bRunning{ false }; // ���� �÷���
	std::thread       m_GameThread;        // ���� ���� ������
	std::chrono::milliseconds m_Tick{ 50 }; // �⺻ ƽ ����

};

extern CGameManager g_GameManager;

