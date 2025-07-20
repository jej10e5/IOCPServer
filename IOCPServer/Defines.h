#pragma once

// ���� �� ���� �� ����
#define MAX_BUFFER_SIZE      4096
#define MAX_CLIENT_COUNT     10000
#define MAX_WORKER_THREADS   8

// ��Ʈ ������ (�׽�Ʈ��)
#define DEFAULT_PORT         7777

// �α� �� ����� ��ũ��
#define LOG(x) std::cout << x << std::endl;
#define ERROR_LOG(x) std::cerr << "[ERROR] " << x << std::endl;
