#pragma once
#include "../ServerCommon/pch.h"
#include <functional>
#include <cstdint>

struct LoginResult 
{
    int64_t userId = 0;
    int     code = -1; 
};
