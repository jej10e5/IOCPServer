#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "../ServerCommon/PacketStruct.h"
#pragma comment(lib, "ws2_32.lib")

int main()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		std::cerr << "WSAStartup 실패" << std::endl;
		return -1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "socket 생성 실패" << std::endl;
		return -1;
	}

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(GATE_PORT);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);  // 로컬 테스트

	if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << "서버 연결 실패: " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	// Echo 패킷 구성
	PACKET_MSG_ECHO pkt;
	strcpy_s(pkt._msg, "Hello from TestClient");

	int len = send(sock, reinterpret_cast<const char*>(&pkt), pkt._header.size, 0);
	if (len == SOCKET_ERROR)
	{
		std::cerr << "send 실패: " << WSAGetLastError() << std::endl;
	}
	else
	{
		std::cout << "Echo 패킷 전송 완료 (" << len << " 바이트)" << std::endl;
	}

	char recvBuf[512] = {};
	int recvLen = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (recvLen > 0)
	{
		const PACKET_MSG_ECHO* pEcho = reinterpret_cast<const PACKET_MSG_ECHO*>(recvBuf);
		std::cout << "서버 응답: " << pEcho->_msg << std::endl;
	}
	else if (recvLen == 0)
	{
		std::cout << "서버 연결 종료됨" << std::endl;
	}
	else
	{
		std::cerr << "recv 실패: " << WSAGetLastError() << std::endl;
	}

	closesocket(sock);
	WSACleanup();
	return 0;
}
