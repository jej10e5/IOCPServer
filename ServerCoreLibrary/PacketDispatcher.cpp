#include "pch.h"
#include "PacketDispatcher.h"
#include "Session.h"
#include "PacketHandlers.h"

void PacketDispatcher::Register(UINT16 _ui16PacketId, HandlerFunc _handler)
{
    auto iter = m_HandlerList.find(_ui16PacketId);
    if (iter != m_HandlerList.end())
    {
        ERROR_LOG("�ߺ� ��Ŷ ID ��� �õ� : " << _ui16PacketId);
        return;
    }

    m_HandlerList.insert(make_pair(_ui16PacketId, _handler));

}

void PacketDispatcher::Dispatch(Session* _pSession, const char* _pData, UINT16 _ui16Size)
{
    //���ŵ� �����͸� ����, ��Ŷ id�� �ش��ϴ� �ڵ鷯 �Լ��� ã�� ����
    if (_ui16Size < PACKET_HEADER_SIZE)
    {
        ERROR_LOG("��Ŷ ���� ���� : " << _ui16Size);
        return;
    }

    // ��Ŷ ���̵� �˾Ƴ��� 
    const PacketHeader* pHeader =reinterpret_cast<const PacketHeader*>(_pData);
    UINT16 uiPacketId = pHeader->id;

    // �ش� ��Ŷ ���̵��� �ڵ鷯 Ȯ��
    auto iter = m_HandlerList.find(uiPacketId);
    if (iter == m_HandlerList.end())
    {
        ERROR_LOG("�� �� ���� Packet Id : " << uiPacketId);
    }
    else
    {
        // �ڵ鷯 ����
        iter->second(_pSession, _pData, _ui16Size);
    }
    
    
}
