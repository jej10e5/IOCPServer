// pch.h
#pragma once

// Windows ���
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>

// ǥ�� ���̺귯��
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

// ������Ʈ ���� ���
#include "Types.h"
#include "Defines.h"

// �����Ϸ� ��� ���� ����
#pragma warning(disable: 4996) // unsafe �Լ� (strcpy ��) ��� ��� ����