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
	char _msg[MAX_CHAT_LENGTH];
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
	char _msg[MAX_CHAT_LENGTH];
};
#pragma pack(pop)


#pragma pack(push,1)
struct CP_LOGIN
{
	CP_LOGIN()
	{
		_header.id = CM_LOGIN;
		_header.size = sizeof(CP_LOGIN);
		memset(&_id, 0x00, sizeof(_id));
		memset(&_pw, 0x00, sizeof(_pw));
	}
	PacketHeader _header;
	char _id[MAX_ID_LENGTH];
	char _pw[MAX_PW_LENGTH];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SP_LOGIN
{
	SP_LOGIN()
	{
		_header.id = SM_LOGIN;
		_header.size = sizeof(SP_LOGIN);
		memset(&_i32Result, 0x00, sizeof(_i32Result));
		memset(&_i64Unique, 0x00, sizeof(_i64Unique));
		memset(&_id, 0x00, sizeof(_id));
		memset(&_pw, 0x00, sizeof(_pw));
	}
	PacketHeader _header;
	INT32 _i32Result;
	INT64 _i64Unique;
	char _id[MAX_ID_LENGTH];
	char _pw[MAX_PW_LENGTH];
};
#pragma pack(pop)



#pragma pack(push,1)
struct CP_CHAT
{
	CP_CHAT()
	{
		_header.id = CM_CHAT;
		_header.size = sizeof(CP_CHAT);
		memset(&_chat, 0x00, sizeof(_chat));
	}
	PacketHeader _header;
	char _chat[MAX_CHAT_LENGTH];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SP_CHAT
{
	SP_CHAT()
	{
		_header.id = SM_CHAT;
		_header.size = sizeof(SP_CHAT);
		memset(&_i64Unique, 0x00, sizeof(_i64Unique));
		memset(&_id, 0x00, sizeof(_id));
		memset(&_chat, 0x00, sizeof(_chat));
	}
	PacketHeader _header;
	INT64 _i64Unique;
	char _id[MAX_ID_LENGTH];
	char _chat[MAX_CHAT_LENGTH];
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