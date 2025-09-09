#include "GameManager.h"
#include"CUnitPC.h"

CGameManager g_GameManager;

void CGameManager::Init()
{
	for (int i = 0;i < 100;i++)
	{
		m_EmptyUnits.emplace_back(new CUnitPC());
	}

}
void CGameManager::Login(UINT64 _ui64Id, char* _pName)
{
	auto pClient = GetEmptyPC();
	pClient->SetName(_pName);
	pClient->SetId(_ui64Id);

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

	// 위치 찾기
	auto it = std::find(m_Users.begin(), m_Users.end(), _pUser);
	if (it == m_Users.end()) {
		return;
	}

	// 마지막 원소와 스왑 후 pop 
	*it = m_Users.back();
	m_Users.pop_back();

	m_EmptyUnits.emplace_back(_pUser);

}
