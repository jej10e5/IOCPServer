#pragma once
#include "pch.h"
#include <functional>
//*----------------------------------
// PacketDispatcher 함수
// 목적 : 패킷 처리 라인과 Session 내부의 IOCP 수신 라인을 구분하기 위함
// 역할 : 파싱된 패킷 ID에 따라 각각의 handler 조합
//*----------------------------------
class Session;
using HandlerFunc = std::function<void(Session*, const char*, UINT16)>;
class PacketDispatcher : public Singleton<PacketDispatcher>
{
	friend class Singleton<PacketDispatcher>;
public:
	void Register(UINT16 _ui16PacketId, HandlerFunc _handler);
	void Dispatch(Session* _pSession, const char* _pData, UINT16 _ui16Size);

private:
	unordered_map<UINT16, HandlerFunc> m_HandlerList;

};

struct PacketHandlerRegistrar
{
	PacketHandlerRegistrar(UINT16 id, HandlerFunc fn)
	{
		PacketDispatcher::GetInstance().Register(id, fn);
	}
};

#define REGISTER_HANDLER(ID, FUNC) static PacketHandlerRegistrar _reg_##ID(ID, FUNC)

