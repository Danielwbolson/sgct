#pragma once
/*
 *	Simple UDP client
 *	https://www.binarytides.com/udp-socket-programming-in-winsock/
 */

#include <WinSock2.h>
#include <ws2tcpip.h>

class UdpSocketTransmitter {
public:
	UdpSocketTransmitter();
	UdpSocketTransmitter(const char addr[INET6_ADDRSTRLEN], int port);

	// Send message to client
	unsigned sendMessage(const char* message, int length);

	// Call once we're done
	void cleanup();

private:
	// Initialize the server
	unsigned init();

	int m_port;
	char m_addr[INET6_ADDRSTRLEN];

	WSADATA m_wsa;
	sockaddr_in m_serverAddr;

	SOCKET m_socket;
	int m_socketLen;
};