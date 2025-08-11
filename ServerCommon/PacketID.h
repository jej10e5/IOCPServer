#pragma once
#include "pch.h"

enum PacketID : UINT16
{
    CM_ECHO = 1,
    CM_CHAT,
    SM_ECHO,
    SM_CHAT,
    CM_LOGIN,
    SM_LOGIN,
};
