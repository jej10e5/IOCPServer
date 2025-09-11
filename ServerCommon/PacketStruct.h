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
		memset(&_name, 0x00, sizeof(_name));
	}
	PacketHeader _header;
	char _name[MAX_NICKNAME_LENGTH];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SP_LOGIN
{
	SP_LOGIN()
	{
		_header.id = SM_LOGIN;
		_header.size = sizeof(SP_LOGIN);
		memset(&_ui64id, 0x00, sizeof(_ui64id));
		memset(&_name, 0x00, sizeof(_name));
	}
	PacketHeader _header;
	UINT64 _ui64id;
	char _name[MAX_NICKNAME_LENGTH];
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
		memset(&_ui64id, 0x00, sizeof(_ui64id));
		memset(&_name, 0x00, sizeof(_name));
		memset(&_chat, 0x00, sizeof(_chat));
	}
	PacketHeader _header;
	UINT64 _ui64id;
	char _name[MAX_NICKNAME_LENGTH];
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