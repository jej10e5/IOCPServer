#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include "../ServerCommon/PacketStruct.h"
#pragma comment(lib, "ws2_32.lib")

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
        std::cerr << "�߸��� ��Ŷ ũ��: " << hdr.size << std::endl;
        return false;
    }

    out.resize(hdr.size);
    // ��� ����
    memcpy(out.data(), &hdr, sizeof(hdr));

    // ���� ����
    int remain = static_cast<int>(hdr.size) - static_cast<int>(sizeof(hdr));
    if (remain > 0) {
        r = RecvAll(s, out.data() + sizeof(hdr), remain);
        if (r <= 0) return false;
    }
    return true;
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup ����\n";
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) { std::cerr << "socket ���� ����\n"; return -1; }

    // (�ɼ�) �ҷ� ��Ŷ �׽�Ʈ �� ������ ���
    BOOL on = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GATE_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "���� ���� ����: " << WSAGetLastError() << "\n";
        closesocket(sock); WSACleanup(); return -1;
    }

    // --- Echo ��û ������ ---
    CP_ECHO pkt{};
    pkt._header.id = CM_ECHO;             // �� ������Ʈ ��� ���
    std::strncpy(pkt._msg, "Hello from TestClient2", sizeof(pkt._msg) - 1);
    pkt._header.size = sizeof(pkt);            // �� �ݵ�� ä���

    int len = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
    if (len == SOCKET_ERROR) {
        std::cerr << "send ����: " << WSAGetLastError() << "\n";
    }
    else {
        std::cout << "Echo ��Ŷ ���� �Ϸ� (" << len << " ����Ʈ)\n";
    }

    // --- ���� �б�(����溻��) & ǥ�� ---
    while (true)
    {
        std::vector<char> recvBuf;
        if (ReadOnePacket(sock, recvBuf)) {
            const PacketHeader* h = reinterpret_cast<const PacketHeader*>(recvBuf.data());
            // ���� ������ ���� ������Ŭ�� ����ü(SP_ECHO)�� ����
            if (h->id == SM_ECHO && recvBuf.size() >= sizeof(SP_ECHO)) {
                const SP_ECHO* pEcho = reinterpret_cast<const SP_ECHO*>(recvBuf.data());
                std::cout << "[���� ����] id=" << h->id
                    << ", size=" << h->size
                    << ", msg=\"" << pEcho->_msg << "\"\n";
            }
            else {
                std::cout << "[���� ����] id=" << h->id
                    << ", size=" << h->size << " (�� �� ���� ��Ŷ)\n";
            }
        }
        else {
            std::cerr << "���� ���� ����(���� ����/����)\n";
        }
    }
    

    // (�ɼ�) ���� �� �ְ�ް� �ʹٸ� ������ ������ ��
    // while (ReadOnePacket(sock, recvBuf)) { ... }

    closesocket(sock);
    WSACleanup();
    return 0;
}
