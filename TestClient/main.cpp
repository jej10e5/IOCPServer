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
#include <io.h>
#include <fcntl.h>
#include <locale>

#include "../ServerCommon/PacketStruct.h"
#pragma comment(lib, "ws2_32.lib")

// -------------------- 전역 동기화/VT 모드 --------------------
std::mutex g_coutMtx;
std::atomic<bool> g_vtEnabled{ false };
UINT64 ui64Id = 0;

// -------------------- VT 모드 --------------------
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

// -------------------- 현재 라인 지우기 --------------------
static void ClearCurrentLine()
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    if (g_vtEnabled.load())
    {
        std::wcout << L"\r\x1b[2K";  // 현재 줄만 지움
        std::wcout.flush();
    }
    else
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(hOut, &csbi))
            return;

        COORD linePos{ 0, csbi.dwCursorPosition.Y };
        DWORD written = 0;
        FillConsoleOutputCharacterW(hOut, L' ', csbi.dwSize.X, linePos, &written);
        SetConsoleCursorPosition(hOut, linePos);
    }
}

// -------------------- UTF-8 <-> UTF-16 변환 --------------------
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
}

static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), len);
    return out;
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
    int r = RecvAll(s, reinterpret_cast<char*>(&hdr), (int)sizeof(hdr));
    if (r <= 0) return false;

    if (hdr.size < sizeof(hdr))
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"잘못된 패킷 크기: " << hdr.size << L"\n";
        return false;
    }

    out.resize(hdr.size);
    memcpy(out.data(), &hdr, sizeof(hdr));

    int remain = (int)hdr.size - (int)sizeof(hdr);
    if (remain > 0)
    {
        r = RecvAll(s, out.data() + sizeof(hdr), remain);
        if (r <= 0) return false;
    }
    return true;
}

// -------------------- 프롬프트/안전 출력 --------------------
static std::wstring g_inputSnapshotW;
static const std::wstring kPromptW = L"> ";

static void RenderPromptW()
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    std::wcout << L"\r\x1b[2K" << kPromptW << g_inputSnapshotW;
    std::wcout.flush();
}

static void SafePrintChatLineW(const std::wstring& line)
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    std::wcout << L"\r\x1b[2K" << line << L"\n";    // 수신 메시지
    std::wcout << kPromptW << g_inputSnapshotW;     // 입력 복원
    std::wcout.flush();
}

// -------------------- 채팅 입력 루프(키 단위/비차단) --------------------
static void ChatLoop(SOCKET sock, std::atomic<bool>& running)
{
    g_inputSnapshotW.clear();
    RenderPromptW();

    while (running.load())
    {
        if (!_kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(8)); continue; }

        wchar_t ch = _getwch();

        if (ch == L'\r' || ch == L'\n')
        {
            std::wstring wline = g_inputSnapshotW;
            g_inputSnapshotW.clear();
            ClearCurrentLine();

            if (wline == L"/quit") { running.store(false); break; }

            // UTF-8로 변환 후 전송
            std::string utf8 = WideToUtf8(wline);

            CP_CHAT pkt{};
            pkt._header.id = CM_CHAT;
            std::strncpy(pkt._chat, utf8.c_str(), sizeof(pkt._chat)-1);
            pkt._header.size = sizeof(pkt);

            int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
            if (sent == SOCKET_ERROR)
            {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::wcerr << L"send 실패: " << WSAGetLastError() << L"\n";
                running.store(false);
                break;
            }

            RenderPromptW();
        }
        else if (ch == 8 /*Backspace*/)
        {
            if (!g_inputSnapshotW.empty())
            {
                g_inputSnapshotW.pop_back();
                RenderPromptW();
            }
        }
        else if (iswprint(ch))
        {
            // 조합 완료된 유니코드 문자를 그대로 누적
            g_inputSnapshotW.push_back(ch);
            RenderPromptW();
        }
        // 화살표 등 기타 키는 필요 시 추가 처리
    }
}

// -------------------- main --------------------
int main()
{
    // 콘솔을 유니코드로 세팅 (화면 출력/표준 입력 모두)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    _setmode(_fileno(stdout), _O_U16TEXT); // wcout 유니코드
    _setmode(_fileno(stdin), _O_U16TEXT); // wcin  유니코드
    _setmode(_fileno(stderr), _O_U16TEXT); // wcerr 유니코드

    // VT ANSI 시퀀스 사용(지우기/커서이동)
    g_vtEnabled.store(EnableVTMode());

    // 1) WinSock 시작
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"WSAStartup 실패\n";
        return -1;
    }

    // 2) 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"socket 생성 실패\n";
        WSACleanup();
        return -1;
    }

    // (옵션) Nagle 끄기
    BOOL on = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    // 3) 서버 접속
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GATE_PORT);     // 프로젝트 상수
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // 필요시 수정

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"서버 연결 실패: " << WSAGetLastError() << L"\n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcout << L"서버 연결 성공\n";
    }

    // 4) 상태 플래그
    std::atomic<bool> running{ true };
    std::atomic<bool> loggedIn{ false };

    // 5) 수신 스레드: 서버에서 오는 패킷 처리
    std::thread recvThread([&]()
        {
            while (running.load())
            {
                std::vector<char> recvBuf;
                if (!ReadOnePacket(sock, recvBuf))
                {
                    std::lock_guard<std::mutex> lk(g_coutMtx);
                    std::wcerr << L"연결 종료/수신 에러\n";
                    running.store(false);
                    break;
                }

                const PacketHeader* h = reinterpret_cast<const PacketHeader*>(recvBuf.data());

                if (h->id == SM_LOGIN && recvBuf.size() >= sizeof(SP_LOGIN))
                {
                    const SP_LOGIN* p = reinterpret_cast<const SP_LOGIN*>(recvBuf.data());
                    loggedIn.store(true);
                    ui64Id = p->_ui64id;

                    std::wstring nameW = Utf8ToWide(p->_name);

                    std::lock_guard<std::mutex> lk(g_coutMtx);
                    std::wcout << L"[LOGIN OK] id=" << p->_ui64id << L", name=" << nameW << L"\n";
                    std::wcout << L"----------------------------------------\n";
                }
                else if (h->id == SM_CHAT && recvBuf.size() >= sizeof(SP_CHAT))
                {
                    const SP_CHAT* p = reinterpret_cast<const SP_CHAT*>(recvBuf.data());
                    std::wstring nameW = Utf8ToWide(p->_name);
                    std::wstring chatW = Utf8ToWide(p->_chat);

                    if (p->_ui64id == ui64Id)
                        SafePrintChatLineW(L"[CHAT][ME] " + nameW + L" : " + chatW);
                    else
                        SafePrintChatLineW(L"[CHAT] " + nameW + L" : " + chatW);
                }
                else
                {
                    std::lock_guard<std::mutex> lk(g_coutMtx);
                    std::wcout << L"[서버 응답] id=" << h->id << L", size=" << h->size << L" (처리 대상 아님)\n";
                }
            }
        });

    // 6) 로그인 한 번만 입력/전송 (wide 입력 사용)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcout << L"채팅에 참여하려면 닉네임을 입력하세요 : ";
    }

    std::wstring nameLineW;
    if (!std::getline(std::wcin, nameLineW))
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"입력이 취소되었습니다.\n";
        running.store(false);
    }
    else if (running.load())
    {
        std::string utf8name = WideToUtf8(nameLineW);

        CP_LOGIN pkt{};
        std::strncpy(pkt._name, utf8name.c_str(), sizeof(pkt._name) - 1);

        int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
        if (sent == SOCKET_ERROR)
        {
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::wcerr << L"send 실패: " << WSAGetLastError() << L"\n";
            running.store(false);
        }
        else
        {
            ClearCurrentLine();
            {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::wcout << L"로그인 요청 전송… 서버 응답 대기 중\n";
            }
        }
    }

    // 7) 로그인 완료될 때까지 대기
    while (running.load() && !loggedIn.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 8) 로그인 성공했으면 채팅 루프 시작
    if (running.load() && loggedIn.load())
        ChatLoop(sock, running);

    // 9) 정리
    running.store(false);
    shutdown(sock, SD_BOTH);
    if (recvThread.joinable()) recvThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
