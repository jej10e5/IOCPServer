#pragma once

#include "../ServerCommon/pch.h"

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

#ifdef  _DEBUG
#pragma comment(lib,"..\\Libraries\\Debug\\ServerCommon.lib")
#else
#pragma comment(lib,"..\\Libraries\\Release\\ServerCommon.lib")
#endif //  _DEBUG
