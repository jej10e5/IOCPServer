#pragma once

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�

using namespace std;

// Windows API
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>

// C/C++ ǥ�� ���̺귯��
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <stack>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>
#include <cassert>
#include <set>
#include <unordered_set>



// ������Ʈ ���� ���
#include "Types.h"
#include "Defines.h"
#include "PacketID.h"
#include "PacketStruct.h"
#include "Singleton.h"

// �����Ϸ� ��� ���� (��: CRT ���� ���)
#pragma warning(disable: 4996)
#pragma comment(lib, "Ws2_32.lib")
