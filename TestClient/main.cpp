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
        if (r <= 0) return r; // 0: 종료, <0: 에러
        got += r;
    }
    return got;
}

static bool ReadOnePacket(SOCKET s, std::vector<char>& out) {
    PacketHeader hdr{};
    int r = RecvAll(s, reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (r <= 0) return false;

    if (hdr.size < sizeof(hdr)) {
        std::cerr << "잘못된 패킷 크기: " << hdr.size << std::endl;
        return false;
    }

    out.resize(hdr.size);
    // 헤더 복사
    memcpy(out.data(), &hdr, sizeof(hdr));

    // 본문 수신
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
        std::cerr << "WSAStartup 실패\n";
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) { std::cerr << "socket 생성 실패\n"; return -1; }

    // (옵션) 소량 패킷 테스트 시 반응성 향상
    BOOL on = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GATE_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "서버 연결 실패: " << WSAGetLastError() << "\n";
        closesocket(sock); WSACleanup(); return -1;
    }

    // --- Echo 요청 보내기 ---
    CP_ECHO pkt{};
    pkt._header.id = CM_ECHO;             // 네 프로젝트 상수 사용
    std::strncpy(pkt._msg, "Hello from TestClient2", sizeof(pkt._msg) - 1);
    pkt._header.size = sizeof(pkt);            // ★ 반드시 채우기

    int len = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
    if (len == SOCKET_ERROR) {
        std::cerr << "send 실패: " << WSAGetLastError() << "\n";
    }
    else {
        std::cout << "Echo 패킷 전송 완료 (" << len << " 바이트)\n";
    }

    // --- 응답 읽기(헤더→본문) & 표시 ---
    while (true)
    {
        std::vector<char> recvBuf;
        if (ReadOnePacket(sock, recvBuf)) {
            const PacketHeader* h = reinterpret_cast<const PacketHeader*>(recvBuf.data());
            // 에코 응답은 보통 서버→클라 구조체(SP_ECHO)로 가정
            if (h->id == SM_ECHO && recvBuf.size() >= sizeof(SP_ECHO)) {
                const SP_ECHO* pEcho = reinterpret_cast<const SP_ECHO*>(recvBuf.data());
                std::cout << "[서버 응답] id=" << h->id
                    << ", size=" << h->size
                    << ", msg=\"" << pEcho->_msg << "\"\n";
            }
            else {
                std::cout << "[서버 응답] id=" << h->id
                    << ", size=" << h->size << " (알 수 없는 패킷)\n";
            }
        }
        else {
            std::cerr << "응답 수신 실패(연결 종료/에러)\n";
        }
    }
    

    // (옵션) 여러 번 주고받고 싶다면 루프로 돌리면 됨
    // while (ReadOnePacket(sock, recvBuf)) { ... }

    closesocket(sock);
    WSACleanup();
    return 0;
}
