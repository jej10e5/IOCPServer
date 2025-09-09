#include "CUnitPC.h"

void CUnitPC::Init()
{
	m_ui64Id = 0;
	memset(m_cName, 0x00, sizeof(m_cName));
}

void CUnitPC::SetName(char* _pName)
{
	memcpy(m_cName, _pName, sizeof(m_cName));
	m_cName[255] = 0;
}

void CUnitPC::GetName(char* _pName)
{
	memcpy(_pName, m_cName, sizeof(m_cName));
}

