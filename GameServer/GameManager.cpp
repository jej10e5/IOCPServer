#include "GameManager.h"
#include"CUnitPC.h"
#include "ClientSession.h"
#include "GameDispatcher.h"

CGameManager g_GameManager;

void CGameManager::Init()
{
	for (int i = 0;i < 100;i++)
	{
		m_EmptyUnits.emplace_back(new CUnitPC());
	}

}
// GameManager.cpp
bool CGameManager::StartLoop(std::chrono::milliseconds tick)
{
	bool expected = false;
	if (!m_bRunning.compare_exchange_strong(expected, true))
		return false; // �̹� ���� ��

	m_Tick = tick;
	m_GameThread = std::thread(&CGameManager::RunLoop, this);
	return true;
}

void CGameManager::StopLoop()
{
	bool expected = true;
	if (!m_bRunning.compare_exchange_strong(expected, false))
		return; // �̹� ����

	if (m_GameThread.joinable())
		m_GameThread.join();
}

void CGameManager::RunLoop()
{
	using clock = std::chrono::steady_clock;
	auto next = clock::now();

	while (m_bRunning) {
		// 1) ��Ʈ��ũ/�ٸ� �����忡�� �ö�� ���� ���� �����忡�� ó��
		GameDispatcher::Instance().Drain(2000); // �����Ӵ� ó�� �ѵ�

		// 2) ������ ƽ
		// ���� ������ ������ ���⼭ ������

		// 3) ���� �ð� ���� ����
		next += m_Tick;
		std::this_thread::sleep_until(next);
	}

	// ���� ���� ���� �۾� �� �� �� ���� ������:
	GameDispatcher::Instance().Drain(100000);
}

void CGameManager::Login(Session* _pSession, char* _pName)
{
	std::scoped_lock lk(m_UserMtx);
	auto pClient = GetEmptyPC();
	pClient->SetName(_pName);
	pClient->SetId(_pSession->GetSessionId());

	// GameManager.cpp - �α��� �� �ݹ� ���� �κ�
	_pSession->SetOnDisconnect([this](UINT64 sid) {
		GameDispatcher::Instance().Post([this, sid] {
			this->Logout(sid);
			});
		});

	m_SessionIdToPC[_pSession->GetSessionId()] = pClient;

}

void CGameManager::Logout(UINT64 _ui64Id)
{
	std::scoped_lock lk(m_UserMtx);

	auto itPC = m_SessionIdToPC.find(_ui64Id);
	if (itPC != m_SessionIdToPC.end()) 
	{
		CUnitPC* pPC = itPC->second;
		m_SessionIdToPC.erase(itPC);
		ReleasePC(pPC); // ���ο��� Init() �� �� Ǯ��
	}


}

CUnitPC* CGameManager::FindUserById(UINT64 _ui64Id)
{
	for (auto user : m_Users)
	{
		if (user->GetId() == _ui64Id)
			return user;
	}

	return nullptr;
}

CUnitPC* CGameManager::GetEmptyPC()
{
	if (m_EmptyUnits.empty())
	{
		for (int i = 0;i < 10;i++)
		{
			m_EmptyUnits.emplace_back(new CUnitPC());
		}
	}

	auto pEmpty = m_EmptyUnits.back();
	m_Users.emplace_back(pEmpty);
	m_EmptyUnits.pop_back();

	return pEmpty;
}

void CGameManager::ReleasePC(CUnitPC* _pUser)
{
	if (_pUser == NULL)
		return;

	// ��ġ ã��
	auto it = std::find(m_Users.begin(), m_Users.end(), _pUser);
	if (it == m_Users.end()) {
		return;
	}

	// ������ ���ҿ� ���� �� pop 
	*it = m_Users.back();
	m_Users.pop_back();

	_pUser->Init();
	m_EmptyUnits.emplace_back(_pUser);

}
