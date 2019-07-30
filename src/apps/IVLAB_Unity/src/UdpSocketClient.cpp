#include <stdio.h>
#include <iostream>
#include "UdpSocketClient.h"

const char DEFAULT_HOST[INET6_ADDRSTRLEN] = "128.101.106.85";
const int DEFAULT_PORT = 5001;

UdpSocketClient::UdpSocketClient() : UdpSocketClient(DEFAULT_HOST, DEFAULT_PORT) {};

UdpSocketClient::UdpSocketClient(const char addr[INET6_ADDRSTRLEN], int port) {
	m_port = port;
	strcpy_s(m_addr, addr);

	init();
}

void UdpSocketClient::cleanup() {
	closesocket(m_socket);
	WSACleanup();
}

unsigned UdpSocketClient::init() {
	m_socket, m_socketLen = sizeof(m_serverAddr);

	//Initialise winsock
	std::cerr << "Initialising Winsock..." << std::endl;
	if (WSAStartup(MAKEWORD(2, 2), &m_wsa) != 0)
	{
		std::cerr << "Failed. Error Code: " << WSAGetLastError() << std::endl;
		return 1;
	}
	std::cout << "Initialised" << std::endl;

	//create socket
	if ((m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		std::cerr << " socket() failed with error code: " << WSAGetLastError() << std::endl;
		return 1;
	}

	//setup address structure
	memset((char *)&m_serverAddr, 0, sizeof(m_serverAddr));
	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_port = htons(m_port);
	int outIP;
	inet_pton(AF_INET, (PSTR)m_addr, &outIP);
	m_serverAddr.sin_addr.S_un.S_addr = outIP;

}

unsigned UdpSocketClient::sendMessage(const char* message, int length) {
	//send the message
	if (sendto(m_socket, message, length, 0, (struct sockaddr *) &m_serverAddr, m_socketLen) == SOCKET_ERROR)
	{
		std::cerr << " sendto() failed with error code: " << WSAGetLastError() << std::endl;
		return 1;
	}
}

unsigned UdpSocketClient::receiveMessage(char* message, const int length) {
	// receive the message
	if (recvfrom(m_socket, message, length, 0, (struct sockaddr *) &m_serverAddr, 0) == SOCKET_ERROR) {
		std::cerr << " recvfrom() failed with error code: " << WSAGetLastError() << std::endl;
		return 1;
	}
}
