#pragma once
#include "pch.h"

#pragma pack(push,1)
struct PacketHeader
{
	UINT16 size;
	UINT16 id;
};
#pragma pack(pop)

#pragma pack(push,1)
struct CP_ECHO
{
	CP_ECHO()
	{
		_header.id = CM_ECHO;
		_header.size = sizeof(CP_ECHO);
		memset(&_msg, 0x00, sizeof(_msg));
	}
	PacketHeader _header;
	char _msg[256];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SP_ECHO
{
	SP_ECHO()
	{
		_header.id = SM_ECHO;
		_header.size = sizeof(SP_ECHO);
		memset(&_msg, 0x00, sizeof(_msg));
	}
	PacketHeader _header;
	char _msg[256];
};
#pragma pack(pop)


#pragma pack(push,1)
struct CP_LOGIN
{
	CP_LOGIN()
	{
		_header.id = CM_CHAT;
		_header.size = sizeof(CP_LOGIN);
		memset(&_id, 0x00, sizeof(_id));
		memset(&_pw, 0x00, sizeof(_pw));
	}
	PacketHeader _header;
	char _id[256];
	char _pw[256];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SP_LOGIN
{
	SP_LOGIN()
	{
		_header.id = CM_CHAT;
		_header.size = sizeof(SP_LOGIN);
		memset(&_i32Result, 0x00, sizeof(_i32Result));
		memset(&_i32Accunique, 0x00, sizeof(_i32Accunique));
	}
	PacketHeader _header;
	INT32 _i32Result;
	INT32 _i32Accunique;
};
#pragma pack(pop)



#pragma pack(push,1)
struct DCP_ECHO
{
	DCP_ECHO()
	{
		_header.id = DCM_ECHO;
		_header.size = sizeof(DCP_ECHO);
		memset(&_msg, 0x00, sizeof(_msg));
	}
	PacketHeader _header;
	char _msg[256];
};
#pragma pack(pop)

#pragma pack(push,1)
struct DSP_ECHO
{
	DSP_ECHO()
	{
		_header.id = DSM_ECHO;
		_header.size = sizeof(DSP_ECHO);
		memset(&_msg, 0x00, sizeof(_msg));
	}
	PacketHeader _header;
	char _msg[256];
};
#pragma pack(pop)