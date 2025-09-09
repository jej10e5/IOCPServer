#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <limits>
#include <cstring>
#include <mutex>

#include "../ServerCommon/PacketStruct.h"
#pragma comment(lib, "ws2_32.lib")

// -------------------- ���� ����ȭ/VT ��� --------------------
std::mutex g_coutMtx;
std::atomic<bool> g_vtEnabled{ false };

UINT64 ui64Id;

static bool EnableVTMode() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return false;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, mode)) return false;
    return true;
}

static void ClearLastInputLine() {
    std::lock_guard<std::mutex> lk(g_coutMtx);
    if (g_vtEnabled.load()) {
        // �� �� ���� �̵� �� �� �� �����
        std::cout << "\x1b[1A\x1b[2K";
        std::cout.flush();
    }
    else {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
        if (csbi.dwCursorPosition.Y == 0) return; // ù ���̸� �н�
        COORD linePos{ 0, static_cast<SHORT>(csbi.dwCursorPosition.Y - 1) };
        DWORD written = 0;
        SetConsoleCursorPosition(hOut, linePos);
        FillConsoleOutputCharacterA(hOut, ' ', csbi.dwSize.X, linePos, &written);
        SetConsoleCursorPosition(hOut, linePos);
    }
}

// -------------------- Recv ��ƿ --------------------
static int RecvAll(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int r = recv(s, buf + got, len - got, 0);
        if (r <= 0) return r; // 0: ����, <0: ����
        got += r;
    }
    return got;
}

static bool ReadOnePacket(SOCKET s, std::vector<char>& out) {
    PacketHeader hdr{};
    int r = RecvAll(s, reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (r <= 0) return false;

    if (hdr.size < sizeof(hdr)) {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "�߸��� ��Ŷ ũ��: " << hdr.size << std::endl;
        return false;
    }

    out.resize(hdr.size);
    memcpy(out.data(), &hdr, sizeof(hdr));

    int remain = static_cast<int>(hdr.size) - static_cast<int>(sizeof(hdr));
    if (remain > 0) {
        r = RecvAll(s, out.data() + sizeof(hdr), remain);
        if (r <= 0) return false;
    }
    return true;
}

// -------------------- ä�� �Է� ���� --------------------
static void ChatLoop(SOCKET sock, std::atomic<bool>& running) {
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cout << "���� ä���� �Է��ϼ���. ����� /quit\n";
    }
    for (std::string line; running.load() && std::getline(std::cin, line); ) {
        if (line == "/quit") {
            running.store(false);
            break;
        }

        CP_CHAT pkt{};
        pkt._header.id = CM_CHAT;
        std::strncpy(pkt._chat, line.c_str(), sizeof(pkt._chat) - 1);
        pkt._header.size = sizeof(pkt);

        int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
        if (sent == SOCKET_ERROR) {
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::cerr << "send ����: " << WSAGetLastError() << "\n";
            running.store(false);
            break;
        }
        else {
            // ���� ģ �Է� �� �����
            ClearLastInputLine();
        }
    }
}

int main() {
    // �ܼ� VT ��� Ȱ�� �õ�
    g_vtEnabled.store(EnableVTMode());

    // 1) WinSock ����
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "WSAStartup ����\n";
        return -1;
    }

    // 2) ���� ����
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "socket ���� ����\n";
        WSACleanup();
        return -1;
    }

    // (�ɼ�) Nagle ����
    BOOL on = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    // 3) ���� ����
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GATE_PORT);          // ������Ʈ ���
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "���� ���� ����: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cout << "���� ���� ����\n";
    }

    // 4) ���� �÷���
    std::atomic<bool> running{ true };
    std::atomic<bool> loggedIn{ false };

    // 5) ���� ������: �������� ���� ��Ŷ ó��
    std::thread recvThread([&]() {
        while (running.load()) {
            std::vector<char> recvBuf;
            if (!ReadOnePacket(sock, recvBuf)) {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cerr << "���� ����/���� ����\n";
                running.store(false);
                break;
            }

            const PacketHeader* h = reinterpret_cast<const PacketHeader*>(recvBuf.data());

            if (h->id == SM_LOGIN && recvBuf.size() >= sizeof(SP_LOGIN)) {
                const SP_LOGIN* p = reinterpret_cast<const SP_LOGIN*>(recvBuf.data());
                loggedIn.store(true);
                std::lock_guard<std::mutex> lk(g_coutMtx);
                ui64Id = p->_ui64id;
                std::cout << "[LOGIN OK] id=" << p->_ui64id << ", name=" << p->_name << "\n";
                std::cout << "----------------------------------------\n";
            }
            else if (h->id == SM_CHAT && recvBuf.size() >= sizeof(SP_CHAT)) {
                const SP_CHAT* p = reinterpret_cast<const SP_CHAT*>(recvBuf.data());
                std::lock_guard<std::mutex> lk(g_coutMtx);
                if(p->_ui64id == ui64Id)
                    std::cout << "[CHAT][ME] " << p->_name << " : " << p->_chat << "\n";
                else
                    std::cout << "[CHAT] " << p->_name << " : " << p->_chat << "\n";
            }
            else {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cout << "[���� ����] id=" << h->id << ", size=" << h->size << " (ó�� ��� �ƴ�)\n";
            }
        }
        });

    // 6) �α��� �� ���� �Է�/����
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cout << "ä�ÿ� �����Ϸ��� �г����� �Է��ϼ��� : ";
    }
    std::string nameLine;
    if (!std::getline(std::cin, nameLine)) {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "�Է��� ��ҵǾ����ϴ�.\n";
        running.store(false);
    }
    else if (running.load()) {
        CP_LOGIN pkt{};
        std::strncpy(pkt._name, nameLine.c_str(), sizeof(pkt._name) - 1);

        int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
        if (sent == SOCKET_ERROR) {
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::cerr << "send ����: " << WSAGetLastError() << "\n";
            running.store(false);
        }
        else {
            // �г��� �Է� �� ����
            ClearLastInputLine();
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::cout << "�α��� ��û ���ۡ� ���� ���� ��� ��\n";
        }
    }

    // 7) �α��� �Ϸ�� ������ ���
    while (running.load() && !loggedIn.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 8) �α��� ���������� ä�� ���� ����
    if (running.load() && loggedIn.load()) {
        ChatLoop(sock, running);
    }

    // 9) ����
    running.store(false);
    shutdown(sock, SD_BOTH);
    if (recvThread.joinable()) recvThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
