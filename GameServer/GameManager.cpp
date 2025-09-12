#include "GameManager.h"
#include"CUnitPC.h"
#include "ClientSession.h"
#include "GameDispatcher.h"

CGameManager g_GameManager;

void CGameManager::Init()
{
	for (int i = 0;i < POOL_SIZE_PC;i++)
	{
		m_EmptyUnits.emplace_back(new CUnitPC());
	}

}
// GameManager.cpp
bool CGameManager::StartLoop(std::chrono::milliseconds tick)
{
	bool expected = false;
	if (!m_bRunning.compare_exchange_strong(expected, true))
		return false; // 이미 실행 중

	m_Tick = tick;
	m_GameThread = std::thread(&CGameManager::RunLoop, this);
	return true;
}

void CGameManager::StopLoop()
{
	bool expected = true;
	if (!m_bRunning.compare_exchange_strong(expected, false))
		return; // 이미 정지

	if (m_GameThread.joinable())
		m_GameThread.join();
}

void CGameManager::RunLoop()
{
	using clock = std::chrono::steady_clock;
	auto next = clock::now();

	while (m_bRunning) 
	{
		// 1) 네트워크/다른 스레드에서 올라온 일을 게임 스레드에서 처리
		GameDispatcher::GetInstance().Drain(2000); // 프레임당 처리 한도

		// 2) 도메인 틱
		// 루프 돌릴거 있으면 여기서 돌리기

		// 3) 고정 시간 스텝 유지
		next += m_Tick;
		std::this_thread::sleep_until(next);
	}

	// 종료 직전 남은 작업 한 번 더 비우고 싶으면:
	GameDispatcher::GetInstance().Drain(100000);
}

void CGameManager::Login(Session* _pSession, const INT64 _ui64Unique, const char* _pName)
{
	std::scoped_lock lk(m_UserMtx);
	auto pClient = GetEmptyPC();
	pClient->SetName(_pName);
	pClient->SetSessionId(_pSession->GetSessionId());
	pClient->SetUnique(_ui64Unique);
	// GameManager.cpp - 로그인 시 콜백 설정 부분
	_pSession->SetOnDisconnect([this](UINT64 sid) {
		GameDispatcher::GetInstance().Post([this, sid] {
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
		ReleasePC(pPC); // 내부에서 Init() 후 빈 풀로
	}


}

CUnitPC* CGameManager::FindUserById(UINT64 _ui64Id)
{
	for (auto user : m_Users)
	{
		if (user->GetSessionId() == _ui64Id)
			return user;
	}

	return nullptr;
}

CUnitPC* CGameManager::FindUserByUnique(INT64 _i64unique)
{
	for (auto user : m_Users)
	{
		if (user->GetUnique() == _i64unique)
			return user;
	}

	return nullptr;
}

CUnitPC* CGameManager::GetEmptyPC()
{
	if (m_EmptyUnits.empty())
	{
		for (int i = 0;i < POOL_SIZE_PC_EXTRA;i++)
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

	// 위치 찾기
	auto it = std::find(m_Users.begin(), m_Users.end(), _pUser);
	if (it == m_Users.end()) {
		return;
	}

	// 마지막 원소와 스왑 후 pop 
	*it = m_Users.back();
	m_Users.pop_back();

	_pUser->Init();
	m_EmptyUnits.emplace_back(_pUser);

}
