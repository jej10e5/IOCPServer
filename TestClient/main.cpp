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

// -------------------- ���� ����ȭ/VT ��� --------------------
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
        std::cout << "\r\x1b[2K"; // Ŀ���� �ִ� ���� �ٸ� ����
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
    int r = RecvAll(s, reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (r <= 0) return false;

    if (hdr.size < sizeof(hdr)) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "�߸��� ��Ŷ ũ��: " << hdr.size << std::endl;
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

static std::string g_inputSnapshot; // ���� ����ڰ� Ÿ���� ���� ����
static const std::string kPrompt = "> ";

static void RenderPrompt()
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    std::cout << "\r\x1b[2K" << kPrompt << g_inputSnapshot;
    std::cout.flush();
}

// ���� �����尡 ��ȭ �� ���� ���� �� ���
static void SafePrintChatLine(const std::string& line)
{
    std::lock_guard<std::mutex> lk(g_coutMtx);
    std::cout << "\r\x1b[2K" << line << "\n"; // ���� ���� �޽��� ���
    std::cout << kPrompt << g_inputSnapshot;  // �� ���� ������Ʈ + �Է� ����
    std::cout.flush();
}


// -------------------- ä�� �Է� ���� --------------------
static void ChatLoop(SOCKET sock, std::atomic<bool>& running)
{
    g_inputSnapshot.clear();
    RenderPrompt();

    while (running.load())
    {
        if (!_kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(8)); continue; }

        wchar_t ch = _getwch();

        // Enter: ����
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
                std::cerr << "send ����: " << WSAGetLastError() << "\n";
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
        // �Ϲ� �μ� ����
        else if (iswprint(ch))
        {
            if (ch < 128) // ������ ASCII�� ���(���ϸ� Ȯ�� ����)
            {
                g_inputSnapshot.push_back(static_cast<char>(ch));
                RenderPrompt();
            }
        }
        // �� �� Ű(ȭ��ǥ ��) �ʿ� �� Ȯ�� ����
    }
}


int main() {
    // �ܼ� VT ��� Ȱ�� �õ�
    g_vtEnabled.store(EnableVTMode());

    // 1) WinSock ����
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "WSAStartup ����\n";
        return -1;
    }

    // 2) ���� ����
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) 
    {
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

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
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
    std::thread recvThread([&](){
        while (running.load()) 
        {
            std::vector<char> recvBuf;
            if (!ReadOnePacket(sock, recvBuf)) 
            {

                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cerr << "���� ����/���� ����\n";
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
    if (!std::getline(std::cin, nameLine)) 
    {
        std::lock_guard<std::mutex> lk(g_coutMtx);
        std::cerr << "�Է��� ��ҵǾ����ϴ�.\n";
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
            std::cerr << "send ����: " << WSAGetLastError() << "\n";
            running.store(false);
        }
        else 
        {
            // �г��� �Է� �� ����
           // �г��� ���� ���� ��
            ClearCurrentLine();
            {
                std::lock_guard<std::mutex> lk(g_coutMtx);
                std::cout << "�α��� ��û ���ۡ� ���� ���� ��� ��\n";
            }

        }
    }

    // 7) �α��� �Ϸ�� ������ ���
    while (running.load() && !loggedIn.load()) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 8) �α��� ���������� ä�� ���� ����
    if (running.load() && loggedIn.load()) 
    {
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
