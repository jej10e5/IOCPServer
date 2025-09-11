#pragma once

// ���� �� ���� �� ����
#define MAX_BUFFER_SIZE      4096
#define MAX_CLIENT_COUNT     10000
#define MAX_WORKER_THREADS   8

// ��Ʈ ������ (�׽�Ʈ��)
#define DEFAULT_PORT         7777
#define GATE_PORT         14001

// �α� �� ����� ��ũ��
#define LOG(x) std::cout << x << std::endl;
#define ERROR_LOG(x) std::cerr << "[ERROR] " << x << std::endl;
#pragma once

#define MAX_SESSION_SIZE	1000

#define PACKET_LENGTH_SIZE 2
#define PACKET_ID_SIZE 2
#define PACKET_HEADER_SIZE (PACKET_LENGTH_SIZE + PACKET_ID_SIZE)
#define MAX_PACKET_SIZE UINT16_MAX
#define MAX_RECV_BUFFER_SIZE (8192)

#define MAX_NICKNAME_LENGTH 256
#define MAX_CHAT_LENGTH 1000

#define POOL_SIZE_PC	100
#define POOL_SIZE_PC_EXTRA (POOL_SIZE_PC/10)
