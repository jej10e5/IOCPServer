#include "pch.h"
#include "PacketDispatcher.h"
#include "Session.h"
#include "PacketHandlers.h"

void PacketDispatcher::Register(UINT16 _ui16PacketId, HandlerFunc _handler)
{
    auto iter = m_HandlerList.find(_ui16PacketId);
    if (iter != m_HandlerList.end())
    {
        ERROR_LOG("중복 패킷 ID 등록 시도 : " << _ui16PacketId);
        return;
    }

    m_HandlerList.insert(make_pair(_ui16PacketId, _handler));

}

void PacketDispatcher::Dispatch(Session* _pSession, const char* _pData, UINT16 _ui16Size)
{
    //수신된 데이터를 보고, 패킷 id에 해당하는 핸들러 함수를 찾아 실행
    if (_ui16Size < PACKET_HEADER_SIZE)
    {
        ERROR_LOG("패킷 길이 부족 : " << _ui16Size);
        return;
    }

    // 패킷 아이디 알아내기 
    const PacketHeader* pHeader =reinterpret_cast<const PacketHeader*>(_pData);
    UINT16 uiPacketId = pHeader->id;

    // 해당 패킷 아이디의 핸들러 확인
    auto iter = m_HandlerList.find(uiPacketId);
    if (iter == m_HandlerList.end())
    {
        ERROR_LOG("알 수 없는 Packet Id : " << uiPacketId);
    }
    else
    {
        // 핸들러 실행
        iter->second(_pSession, _pData, _ui16Size);
    }
    
    
}
