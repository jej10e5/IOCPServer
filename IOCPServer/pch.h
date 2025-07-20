// pch.h
#pragma once

// Windows 헤더
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>

// 표준 라이브러리
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>
#include <cassert>

// 프로젝트 공통 헤더
#include "Types.h"
#include "Defines.h"

// 컴파일러 경고 무시 설정
#pragma warning(disable: 4996) // unsafe 함수 (strcpy 등) 사용 경고 제거