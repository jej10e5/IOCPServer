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
#include <conio.h>   // _kbhit, _getwch
#include <cwctype>   // iswprint


#include "../ServerCommon/PacketStruct.h"
#pragma comment(lib, "ws2_32.lib")

// -------------------- 전역 동기화/VT 모드 --------------------
std::mutex g_coutMtx;
std::atomic<bool> g_vtEnabled{ false };

UINT64 ui64Id;

static bool EnableVTMode() 
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) 
        return false;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) 
        return false;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, mode)) 
        return false;
    return true;
}

static void ClearCurrentLine()
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    if (g_vtEnabled.load()) 
    {
        std::cout << "\r\x1b[2K"; // 커서가 있는 현재 줄만 지움
        std::cout.flush();
    }
    else 
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) 
            return;

        COORD linePos{ 0, csbi.dwCursorPosition.Y };
        DWORD written = 0;
        FillConsoleOutputCharacterA(hOut, ' ', csbi.dwSize.X, linePos, &written);
        SetConsoleCursorPosition(hOut, linePos);
    }
}

// -------------------- Recv 유틸 --------------------
static int RecvAll(SOCKET s, char* buf, int len) 
{
    int got = 0;
    while (got < len) 
    {
        int r = recv(s, buf + got, len - got, 0);
        if (r <= 0) return r; // 0: 종료, <0: 에러
        got += r;
    }
    return got;
}

static bool ReadOnePacket(SOCKET s, std::vector<char>& out) 
{
    PacketHeader hdr{};
    int r = RecvAll(s, reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (r <= 0) return false;

    if (hdr.size < sizeof(hdr)) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "잘못된 패킷 크기: " << hdr.size << std::endl;
        return false;
    }

    out.resize(hdr.size);
    memcpy(out.data(), &hdr, sizeof(hdr));

    int remain = static_cast<int>(hdr.size) - static_cast<int>(sizeof(hdr));
    if (remain > 0) 
    {
        r = RecvAll(s, out.data() + sizeof(hdr), remain);
        if (r <= 0) return false;
    }
    return true;
}

static std::string g_inputSnapshot; // 현재 사용자가 타이핑 중인 내용
static const std::string kPrompt = "> ";

static void RenderPrompt()
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    std::cout << "\r\x1b[2K" << kPrompt << g_inputSnapshot;
    std::cout.flush();
}

// 수신 스레드가 대화 한 줄을 찍을 때 사용
static void SafePrintChatLine(const std::string& line)
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    std::cout << "\r\x1b[2K" << line << "\n"; // 먼저 수신 메시지 출력
    std::cout << kPrompt << g_inputSnapshot;  // 그 다음 프롬프트 + 입력 복원
    std::cout.flush();
}


// -------------------- 채팅 입력 루프 --------------------
static void ChatLoop(SOCKET sock, std::atomic<bool>& running)
{
    g_inputSnapshot.clear();
    RenderPrompt();

    while (running.load())
    {
        if (!_kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(8)); continue; }

        wchar_t ch = _getwch();

        // Enter: 전송
        if (ch == L'\r' || ch == L'\n')
        {
            std::string line = g_inputSnapshot;
            g_inputSnapshot.clear();

            ClearCurrentLine();

            if (line == "/quit") { running.store(false); break; }

            CP_CHAT pkt{};
            pkt._header.id = CM_CHAT;
            std::strncpy(pkt._chat, line.c_str(), sizeof(pkt._chat) - 1);
            pkt._header.size = sizeof(pkt);

            int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
            if (sent == SOCKET_ERROR)
            {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cerr << "send 실패: " << WSAGetLastError() << "\n";
                running.store(false);
                break;
            }

            RenderPrompt();
        }
        // Backspace
        else if (ch == 8 /*Backspace*/)
        {
            if (!g_inputSnapshot.empty())
            {
                g_inputSnapshot.pop_back();
                RenderPrompt();
            }
        }
        // 일반 인쇄 문자
        else if (iswprint(ch))
        {
            if (ch < 128) // 간단히 ASCII만 허용(원하면 확장 가능)
            {
                g_inputSnapshot.push_back(static_cast<char>(ch));
                RenderPrompt();
            }
        }
        // 그 외 키(화살표 등) 필요 시 확장 가능
    }
}


int main() {
    // 콘솔 VT 모드 활성 시도
    g_vtEnabled.store(EnableVTMode());

    // 1) WinSock 시작
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "WSAStartup 실패\n";
        return -1;
    }

    // 2) 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "socket 생성 실패\n";
        WSACleanup();
        return -1;
    }

    // (옵션) Nagle 끄기
    BOOL on = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    // 3) 서버 접속
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GATE_PORT);          // 프로젝트 상수
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "서버 연결 실패: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cout << "서버 연결 성공\n";
    }

    // 4) 상태 플래그
    std::atomic<bool> running{ true };
    std::atomic<bool> loggedIn{ false };

    // 5) 수신 스레드: 서버에서 오는 패킷 처리
    std::thread recvThread([&](){
        while (running.load()) 
        {
            std::vector<char> recvBuf;
            if (!ReadOnePacket(sock, recvBuf)) 
            {

                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cerr << "연결 종료/수신 에러\n";
                running.store(false);
                break;
            }

            const PacketHeader* h = reinterpret_cast<const PacketHeader*>(recvBuf.data());

            if (h->id == SM_LOGIN && recvBuf.size() >= sizeof(SP_LOGIN)) 
            {
                const SP_LOGIN* p = reinterpret_cast<const SP_LOGIN*>(recvBuf.data());
                loggedIn.store(true);
                std::lock_guard<std::mutex> lk(g_coutMtx);
                ui64Id = p->_ui64id;
                std::cout << "[LOGIN OK] id=" << p->_ui64id << ", name=" << p->_name << "\n";
                std::cout << "----------------------------------------\n";
            }
            else if (h->id == SM_CHAT && recvBuf.size() >= sizeof(SP_CHAT)) 
            {
                const SP_CHAT* p = reinterpret_cast<const SP_CHAT*>(recvBuf.data());
                if (p->_ui64id == ui64Id)
                    SafePrintChatLine(std::string("[CHAT][ME] ") + p->_name + " : " + p->_chat);
                else
                    SafePrintChatLine(std::string("[CHAT] ") + p->_name + " : " + p->_chat);
            }
            else 
            {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cout << "[서버 응답] id=" << h->id << ", size=" << h->size << " (처리 대상 아님)\n";
            }
        }
        });

    // 6) 로그인 한 번만 입력/전송
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cout << "채팅에 참여하려면 닉네임을 입력하세요 : ";
    }
    std::string nameLine;
    if (!std::getline(std::cin, nameLine)) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "입력이 취소되었습니다.\n";
        running.store(false);
    }
    else if (running.load()) 
    {
        CP_LOGIN pkt{};
        std::strncpy(pkt._name, nameLine.c_str(), sizeof(pkt._name) - 1);

        int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
        if (sent == SOCKET_ERROR) 
        {
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::cerr << "send 실패: " << WSAGetLastError() << "\n";
            running.store(false);
        }
        else 
        {
            // 닉네임 입력 줄 삭제
           // 닉네임 전송 성공 후
            ClearCurrentLine();
            {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cout << "로그인 요청 전송… 서버 응답 대기 중\n";
            }

        }
    }

    // 7) 로그인 완료될 때까지 대기
    while (running.load() && !loggedIn.load()) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 8) 로그인 성공했으면 채팅 루프 시작
    if (running.load() && loggedIn.load()) 
    {
        ChatLoop(sock, running);
    }

    // 9) 정리
    running.store(false);
    shutdown(sock, SD_BOTH);
    if (recvThread.joinable()) recvThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
