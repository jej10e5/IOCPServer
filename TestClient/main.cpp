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

// -------------------- ���� ����ȭ/VT ��� --------------------
std::mutex g_coutMtx;
std::atomic<bool> g_vtEnabled{ false };
INT64 i64Unique = 0;

// -------------------- VT ��� --------------------
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

// -------------------- ���� ���� ����� --------------------
static void ClearCurrentLine()
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    if (g_vtEnabled.load())
    {
        std::wcout << L"\r\x1b[2K";  // ���� �ٸ� ����
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

// -------------------- UTF-8 <-> UTF-16 ��ȯ --------------------
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

// -------------------- Recv ��ƿ --------------------
static int RecvAll(SOCKET s, char* buf, int len)
{
    int got = 0;
    while (got < len)
    {
        int r = recv(s, buf + got, len - got, 0);
        if (r <= 0) return r; // 0: ����, <0: ����
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
        std::wcerr << L"�߸��� ��Ŷ ũ��: " << hdr.size << L"\n";
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

// -------------------- ������Ʈ/���� ��� --------------------
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
    std::wcout << L"\r\x1b[2K" << line << L"\n";    // ���� �޽���
    std::wcout << kPromptW << g_inputSnapshotW;     // �Է� ����
    std::wcout.flush();
}

// -------------------- ä�� �Է� ����(Ű ����/������) --------------------
// ���� ChatLoop �����ϰ� �� �Լ��� ��ü
static void ChatLoop_LineMode(SOCKET sock, std::atomic<bool>& running)
{
    while (running.load())
    {
        {
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::wcout << L"> " << std::flush;
        }

        std::wstring wline;
        if (!std::getline(std::wcin, wline)) 
        {
            // �Է� ��Ʈ�� ����(�ܼ� ���� ��)
            running.store(false);
            break;
        }

        if (wline == L"/quit") {
            running.store(false);
            break;
        }

        // UTF-8 ��ȯ �� ����
        std::string utf8 = WideToUtf8(wline);

        CP_CHAT pkt{};
        pkt._header.id = CM_CHAT;
        pkt._header.size = sizeof(pkt);
        std::strncpy(pkt._chat, utf8.c_str(), sizeof(pkt._chat) - 1);

        int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
        if (sent == SOCKET_ERROR)
        {
            std::lock_guard<std::mutex> lk2(g_coutMtx);
            std::wcerr << L"send ����: " << WSAGetLastError() << L"\n";
            running.store(false);
            break;
        }
    }
}


// -------------------- main --------------------
int main()
{
    // �ܼ��� �����ڵ�� ���� (ȭ�� ���/ǥ�� �Է� ���)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    _setmode(_fileno(stdout), _O_U16TEXT); // wcout �����ڵ�
    _setmode(_fileno(stdin), _O_U16TEXT); // wcin  �����ڵ�
    _setmode(_fileno(stderr), _O_U16TEXT); // wcerr �����ڵ�

    // VT ANSI ������ ���(�����/Ŀ���̵�)
    g_vtEnabled.store(EnableVTMode());

    // 1) WinSock ����
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"WSAStartup ����\n";
        return -1;
    }

    // 2) ���� ����
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"socket ���� ����\n";
        WSACleanup();
        return -1;
    }

    // (�ɼ�) Nagle ����
    BOOL on = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));

    // 3) ���� ����
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GATE_PORT);     // ������Ʈ ���
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // �ʿ�� ����

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"���� ���� ����: " << WSAGetLastError() << L"\n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcout << L"���� ���� ����\n";
    }

    // 4) ���� �÷���
    std::atomic<bool> running{ true };
    std::atomic<bool> loggedIn{ false };

    // 5) ���� ������: �������� ���� ��Ŷ ó��
    std::thread recvThread([&]()
        {
            while (running.load())
            {
                std::vector<char> recvBuf;
                if (!ReadOnePacket(sock, recvBuf))
                {
                    std::lock_guard<std::mutex> lk(g_coutMtx);
                    std::wcerr << L"���� ����/���� ����\n";
                    running.store(false);
                    break;
                }

                const PacketHeader* h = reinterpret_cast<const PacketHeader*>(recvBuf.data());

                if (h->id == SM_LOGIN && recvBuf.size() >= sizeof(SP_LOGIN))
                {
                    const SP_LOGIN* p = reinterpret_cast<const SP_LOGIN*>(recvBuf.data());
                    if (p->_i32Result != 0)
                    {
                        std::wcout << L"------------------�α��� ����--------------------\n";
                        return 0;
                    }

                    loggedIn.store(true);
                    i64Unique = p->_i64Unique;

                    std::wstring idW = Utf8ToWide(p->_id);
                    std::wstring pwW = Utf8ToWide(p->_pw);

                    std::lock_guard<std::mutex> lk(g_coutMtx);
                    std::wcout << L"[LOGIN OK] unique=" << p->_i64Unique
                        << L", id=" << idW
                        << L", pw=" << pwW << L"\n";
                    std::wcout << L"----------------------------------------\n";
                }

                else if (h->id == SM_CHAT && recvBuf.size() >= sizeof(SP_CHAT))
                {
                    const SP_CHAT* p = reinterpret_cast<const SP_CHAT*>(recvBuf.data());
                    std::wstring nameW = Utf8ToWide(p->_id);
                    std::wstring chatW = Utf8ToWide(p->_chat);

                    if (p->_i64Unique == i64Unique)
                        SafePrintChatLineW(L"[CHAT][ME] " + nameW + L" : " + chatW);
                    else
                        SafePrintChatLineW(L"[CHAT] " + nameW + L" : " + chatW);
                }
                else
                {
                    std::lock_guard<std::mutex> lk(g_coutMtx);
                    std::wcout << L"[���� ����] id=" << h->id << L", size=" << h->size << L" (ó�� ��� �ƴ�)\n";
                }
            }
        });

    // 6) �α��� �Է� (id + pw)
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcout << L"���̵� �Է�: ";
    }
    std::wstring idLineW;
    if (!std::getline(std::wcin, idLineW))
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"�Է��� ��ҵǾ����ϴ�.\n";
        running.store(false);
    }

    std::wstring pwLineW;
    if (running.load())
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcout << L"��й�ȣ �Է�: ";
    }
    if (running.load() && !std::getline(std::wcin, pwLineW))
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::wcerr << L"�Է��� ��ҵǾ����ϴ�.\n";
        running.store(false);
    }

    // 7) ��Ŷ ����
    if (running.load())
    {
        std::string utf8id = WideToUtf8(idLineW);
        std::string utf8pw = WideToUtf8(pwLineW);

        CP_LOGIN pkt{};
        std::strncpy(pkt._id, utf8id.c_str(), sizeof(pkt._id) - 1);
        std::strncpy(pkt._pw, utf8pw.c_str(), sizeof(pkt._pw) - 1);

        int sent = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
        if (sent == SOCKET_ERROR)
        {
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::wcerr << L"send ����: " << WSAGetLastError() << L"\n";
            running.store(false);
        }
        else
        {
            ClearCurrentLine();
            std::lock_guard<std::mutex> lk(g_coutMtx);
            std::wcout << L"�α��� ��û ���ۡ� ���� ���� ��� ��\n";
        }
    }


    // 7) �α��� �Ϸ�� ������ ���
    while (running.load() && !loggedIn.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 8) �α��� ���������� ä�� ���� ����
    if (running.load() && loggedIn.load())
        ChatLoop_LineMode(sock, running);


    // 9) ����
    running.store(false);
    shutdown(sock, SD_BOTH);
    if (recvThread.joinable()) recvThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
