#pragma once
#include "pch.h"

#pragma pack(push,1)
struct PacketHeader
{
	UINT16 size;
	UINT16 id;
};
#pragma pack(pop)

struct PACKET_MSG_ECHO
{
	PACKET_MSG_ECHO()
	{
		_header.id= PACKET_ID_ECHO;
		_header.size = sizeof(PACKET_MSG_ECHO);
		memset(&_msg, 0x00, sizeof(_msg));
	}
	PacketHeader _header;
	char _msg[256];
};