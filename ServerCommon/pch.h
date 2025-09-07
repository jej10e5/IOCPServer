#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다

using namespace std;

// Windows API
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>

// C/C++ 표준 라이브러리
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



// 프로젝트 공용 헤더
#include "Types.h"
#include "Defines.h"
#include "PacketID.h"
#include "PacketStruct.h"
#include "Singleton.h"

// 컴파일러 경고 무시 (예: CRT 보안 경고)
#pragma warning(disable: 4996)
#pragma comment(lib, "Ws2_32.lib")
