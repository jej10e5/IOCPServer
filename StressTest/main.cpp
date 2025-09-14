
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <stop_token>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace std::chrono;

enum class Proto
{
    Raw,
    LenPrefixed,
    Packet // [uint16 size][uint16 id][payload]
};

struct Args
{
    string sHost = "127.0.0.1";
    uint16_t ui16Port = 14002;
    int iConcurrency = 200;
    int iMsgSize = 64;
    int iThinkMs = 0;
    int iDurationSec = 30;
    int iThreads = 0;             // 0이면 HW threads
    Proto eProto = Proto::Raw;
    uint16_t ui16PacketId = 1;    // Packet 모드에서 사용할 ID
};

struct Stats
{
    mutex m;
    vector<double> vRttsMs;       // 개별 RTT(ms)
    atomic<uint64_t> u64Ok{ 0 };
    atomic<uint64_t> u64Err{ 0 };
};

static bool ParseArgs(int argc, char** argv, Args& out)
{
    auto need = [&](int& i) -> string
        {
            if (i + 1 < argc) return string(argv[++i]);
            cerr << "missing value after " << argv[i] << "";
                return string();
        };

    for (int i = 1; i < argc; ++i)
    {
        string sv = argv[i];

        if (sv == "-h" || sv == "--host")
        {
            out.sHost = need(i);
        }
        else if (sv == "-p" || sv == "--port")
        {
            out.ui16Port = static_cast<uint16_t>(stoi(need(i)));
        }
        else if (sv == "-c" || sv == "--concurrency")
        {
            out.iConcurrency = stoi(need(i));
        }
        else if (sv == "-size")
        {
            out.iMsgSize = stoi(need(i));
        }
        else if (sv == "-think")
        {
            out.iThinkMs = stoi(need(i));
        }
        else if (sv == "-dur" || sv == "--duration")
        {
            out.iDurationSec = stoi(need(i));
        }
        else if (sv == "-threads")
        {
            out.iThreads = stoi(need(i));
        }
        else if (sv == "-proto")
        {
            string v = need(i);
            if (v == "raw") out.eProto = Proto::Raw;
            else if (v == "len") out.eProto = Proto::LenPrefixed;
            else if (v == "packet") out.eProto = Proto::Packet;
            else {
                cerr << "unknown proto: " << v ; 
                return false; }
            }
        else if (sv == "-pid" || sv == "--packet-id")
        {
            out.ui16PacketId = static_cast<uint16_t>(stoi(need(i)));
        }
        else
        {
            cerr << "Unknown arg: " << sv ;
                return false;
        }
        }
        return true;
    }

    static SOCKET CreateAndConnectSocket(const string & _sHost, uint16_t _ui16Port)
    {
        SOCKET s = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
        if (s == INVALID_SOCKET)
        {
            return INVALID_SOCKET;
        }

        BOOL bNoDelay = TRUE;
        ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&bNoDelay), sizeof(bNoDelay));

        DWORD dwTimeout = 5000; // 5s
        ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&dwTimeout), sizeof(dwTimeout));
        ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&dwTimeout), sizeof(dwTimeout));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(_ui16Port);
        if (::InetPtonA(AF_INET, _sHost.c_str(), &addr.sin_addr) != 1)
        {
            ::closesocket(s);
            return INVALID_SOCKET;
        }

        if (::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        {
            ::closesocket(s);
            return INVALID_SOCKET;
        }

        return s;
    }

    static bool SendAll(SOCKET s, const char* _pData, int _iLen)
    {
        int iTotal = 0;
        while (iTotal < _iLen)
        {
            int iSent = ::send(s, _pData + iTotal, _iLen - iTotal, 0);
            if (iSent == SOCKET_ERROR) { return false; }
            iTotal += iSent;
        }
        return true;
    }

    static bool RecvExact(SOCKET s, char* _pBuf, int _iLen)
    {
        int iTotal = 0;
        while (iTotal < _iLen)
        {
            int iRecv = ::recv(s, _pBuf + iTotal, _iLen - iTotal, 0);
            if (iRecv <= 0) { return false; }
            iTotal += iRecv;
        }
        return true;
    }

    static void WorkerLoop(std::stop_token _st,
        Stats & _stats,
        const vector<SOCKET>&_vSockets,
        const vector<char>&_vPayload,
        Proto _eProto,
        int _iThinkMs,
        chrono::steady_clock::time_point _tEnd,
        uint16_t _ui16PacketId)
    {
        // 전송 버퍼 준비
        vector<char> vSendBuf;

        if (_eProto == Proto::LenPrefixed)
        {
            vSendBuf.resize(sizeof(uint32_t) + _vPayload.size());
            uint32_t u32Len = static_cast<uint32_t>(_vPayload.size());
            memcpy(vSendBuf.data(), &u32Len, sizeof(u32Len));
            memcpy(vSendBuf.data() + sizeof(u32Len), _vPayload.data(), _vPayload.size());
        }
        else if (_eProto == Proto::Packet)
        {
            // [uint16 size][uint16 id][payload]
            uint16_t ui16Size = static_cast<uint16_t>(sizeof(uint16_t) * 2 + _vPayload.size());
            vSendBuf.resize(ui16Size);
            memcpy(vSendBuf.data(), &ui16Size, sizeof(uint16_t));
            memcpy(vSendBuf.data() + sizeof(uint16_t), &_ui16PacketId, sizeof(uint16_t));
            // payload는 루프에서 매회 채워 넣음(랜덤 페이로드가 변할 수 있으니)
        }

        while (!_st.stop_requested())
        {
            if (steady_clock::now() >= _tEnd) { break; }

            for (SOCKET s : _vSockets)
            {
                if (_st.stop_requested()) { break; }

                auto t0 = steady_clock::now();
                bool bOk = false;

                if (_eProto == Proto::Raw)
                {
                    bOk = SendAll(s, _vPayload.data(), static_cast<int>(_vPayload.size()));
                    if (bOk)
                    {
                        vector<char> tmp(_vPayload.size());
                        bOk = RecvExact(s, tmp.data(), static_cast<int>(tmp.size()));
                    }
                }
                else if (_eProto == Proto::LenPrefixed)
                {
                    bOk = SendAll(s, vSendBuf.data(), static_cast<int>(vSendBuf.size()));
                    if (bOk)
                    {
                        uint32_t u32Len = 0;
                        if (!RecvExact(s, reinterpret_cast<char*>(&u32Len), sizeof(u32Len))) { bOk = false; }
                        else
                        {
                            vector<char> tmp(u32Len);
                            bOk = RecvExact(s, tmp.data(), static_cast<int>(tmp.size()));
                        }
                    }
                }
                else // Packet
                {
                    // payload 채움(매회 바뀔 수 있으므로)
                    memcpy(vSendBuf.data() + sizeof(uint16_t) * 2, _vPayload.data(), _vPayload.size());

                    bOk = SendAll(s, vSendBuf.data(), static_cast<int>(vSendBuf.size()));
                    if (bOk)
                    {
                        // 응답: [size][id][body]
                        uint16_t ui16Size = 0, ui16Id = 0;
                        if (!RecvExact(s, reinterpret_cast<char*>(&ui16Size), sizeof(ui16Size))) { bOk = false; }
                        else if (!RecvExact(s, reinterpret_cast<char*>(&ui16Id), sizeof(ui16Id))) { bOk = false; }
                        else
                        {
                            int iBody = static_cast<int>(ui16Size) - static_cast<int>(sizeof(uint16_t) * 2);
                            if (iBody < 0) { bOk = false; }
                            else
                            {
                                vector<char> tmp(iBody);
                                bOk = RecvExact(s, tmp.data(), static_cast<int>(tmp.size()));
                            }
                        }
                    }
                }

                if (bOk)
                {
                    double ms = duration<double, milli>(steady_clock::now() - t0).count();
                    {
                        std::lock_guard<std::mutex> lg(_stats.m);
                        _stats.vRttsMs.push_back(ms);
                    }
                    _stats.u64Ok.fetch_add(1, memory_order_relaxed);
                }
                else
                {
                    _stats.u64Err.fetch_add(1, memory_order_relaxed);
                }

                if (_iThinkMs > 0) { this_thread::sleep_for(chrono::milliseconds(_iThinkMs)); }
                if (steady_clock::now() >= _tEnd) { break; }
            }
        }
    }

    static pair<double, double> Percentiles(vector<double> v, double p50 = 0.5, double p95 = 0.95)
    {
        if (v.empty()) { return { 0.0, 0.0 }; }
        sort(v.begin(), v.end());
        auto pick = [&](double q)
            {
                size_t i = static_cast<size_t>(q * (v.size() - 1));
                return v[i];
            };
        return { pick(p50), pick(p95) };
    }

    static string NowStamp()
    {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        char buf[64]{};
        sprintf_s(buf, "%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        return string(buf);
    }

    int main(int argc, char** argv)
    {
        ios::sync_with_stdio(false);

        Args args{};
        if (!ParseArgs(argc, argv, args))
        {
            cout << "Usage: Bench.exe -h <host> -p <port> -c <concurrency> -size <bytes> -think <ms> -dur <sec> -threads <n> -proto <raw|len|packet> [-pid <packet_id>]";
                return 1;
        }

        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
            cerr << "WSAStartup failed";
                return 1;
        }

        vector<SOCKET> vSockets;
        vSockets.reserve(args.iConcurrency);

        for (int i = 0; i < args.iConcurrency; ++i)
        {
            SOCKET s = CreateAndConnectSocket(args.sHost, args.ui16Port);
            if (s == INVALID_SOCKET)
            {
                cerr << "connect failed at index " << i ;
            }
            else
            {
                vSockets.push_back(s);
            }
        }

        if (vSockets.empty())
        {
            cerr << "no connected sockets";
            WSACleanup();
            return 1;
        }

        int iThreads = args.iThreads > 0 ? args.iThreads : static_cast<int>(thread::hardware_concurrency());
        iThreads = max(1, min(iThreads, static_cast<int>(vSockets.size())));

        vector<vector<SOCKET>> vBuckets(iThreads);
        for (size_t i = 0; i < vSockets.size(); ++i)
        {
            vBuckets[i % iThreads].push_back(vSockets[i]);
        }

        vector<char> vPayload(args.iMsgSize);
        {
            mt19937 rng(12345);
            uniform_int_distribution<int> dist(0, 255);
            for (auto& c : vPayload) { c = static_cast<char>(dist(rng)); }
        }

        Stats stats{};
        auto tEnd = steady_clock::now() + chrono::seconds(args.iDurationSec);

        std::vector<std::jthread> vThreads;
        vThreads.reserve(iThreads);
        for (int i = 0; i < iThreads; ++i)
        {
            vThreads.emplace_back([&, i](std::stop_token st)
                {
                    WorkerLoop(st,
                    stats, vBuckets[i], vPayload,
                    args.eProto, args.iThinkMs, tEnd,
                    args.ui16PacketId);
                });
        }

        while (steady_clock::now() < tEnd)
        {
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        for (auto& th : vThreads) { th.request_stop(); }
        for (auto& th : vThreads) { if (th.joinable()) th.join(); }

        for (SOCKET s : vSockets) { ::closesocket(s); }

        auto pr = Percentiles(stats.vRttsMs);
        double dSeconds = static_cast<double>(args.iDurationSec);
        double dTps = dSeconds > 0.0 ? static_cast<double>(stats.u64Ok.load()) / dSeconds : 0.0;
        double dErrRate = (stats.u64Ok + stats.u64Err) > 0 ? (static_cast<double>(stats.u64Err.load()) * 100.0 / static_cast<double>(stats.u64Ok.load() + stats.u64Err.load())) : 0.0;

        cout << "OK=" << stats.u64Ok.load() << ", ERR=" << stats.u64Err.load() << ", TPS=" << dTps
            << ", P50(ms)=" << pr.first << ", P95(ms)=" << pr.second << ", ERR%(client)=" << dErrRate;

            string sFile = string("results_") + NowStamp() + ".csv";
        ofstream ofs(sFile, ios::out | ios::trunc);
        if (ofs)
        {
            ofs << "concurrency,size,think_ms,proto,packet_id,tps,p50_ms,p95_ms,err_rate";
            ofs << args.iConcurrency << "," << args.iMsgSize << "," << args.iThinkMs << ","
                << (args.eProto == Proto::Raw ? "raw" : (args.eProto == Proto::LenPrefixed ? "len" : "packet")) << ","
                << (args.eProto == Proto::Packet ? args.ui16PacketId : 0) << ","
                << dTps << "," << pr.first << "," << pr.second << "," << dErrRate ;
            ofs.close();
            cout << "Saved: " << sFile ;
        }
        else
        {
            cerr << "CSV save failed: " << sFile;
        }

        WSACleanup();
        return 0;
    }
