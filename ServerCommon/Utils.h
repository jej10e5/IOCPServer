#pragma once
#include "pch.h"
static std::wstring Utf8ToWide(const char* s) 
{
    if (!s) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring out(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
    return out;
}
