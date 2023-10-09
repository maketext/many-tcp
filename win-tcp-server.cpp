/*
 * Designed for FLIR Industrial Camera (Machine Vision Camera) Develepment Aids on Windows Environment.
 * Designed for Simple TCP Server to get external signal from Equipment Hardware.
 * Many Stallings Company Presents 2023-10-08.
 */

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <functional>
#include <iostream>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define SEND_BUFLEN 16
#define DEFAULT_PORT "8889"

using namespace std;
namespace mssc {
	class Link {
	public:

		Link(const char *to, function<void(char*, int)> go) {
			this->to = to;
			this->go = go;
		};
		const char *to;
		function<void(char*, int)> go;
	};

	class Router {
	public:
		Router() {
		};
		Router(list<Link> &links) {
			this->links = links;
		};
		Router* push(Link &link) {
			this->links.push_back(link);
			return this;
		}
		const char* toString() {
			return "[Object Router]";
		}
		list<Link> links;
	};

	void closeSocket(SOCKET socket) {
		closesocket(socket);
		WSACleanup();
	}

	int onReceive(SOCKET ClientSocket, Router &router) {
		// Receive until the peer shuts down the connection
		int iSendResult, iResult;
		char recvbuf[DEFAULT_BUFLEN];
		int recvbuflen = DEFAULT_BUFLEN;

		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				printf("Bytes received: %d\n", iResult);
				list<Link>::iterator it;
				for (it = router.links.begin(); it != router.links.end(); it++) {
					cout << it->to;
					if (iResult < DEFAULT_BUFLEN)
						recvbuf[iResult] = NULL;
					else
						recvbuf[DEFAULT_BUFLEN - 1] = NULL;
					it->go(recvbuf, iResult);
				}
			}
			else if (iResult == 0)
				printf("Bytes received end.\n");
			else {
				printf("recv failed with error: %d\n", WSAGetLastError());
				closeSocket(ClientSocket);
				return 1;
			}

		} while (iResult > 0);

		// Echo the buffer back to the sender
		char sendbuf[SEND_BUFLEN] = "DATA_PROCESSED";
		int sendbuflen = strlen(sendbuf);

		iSendResult = send(ClientSocket, sendbuf, sendbuflen, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());

			closeSocket(ClientSocket);
			return 1;
		}
		printf("Bytes sent. Content-Length: %d, %d\n", iSendResult, sendbuflen);

		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());

			closeSocket(ClientSocket);
			return 1;
		}

		// cleanup
		closeSocket(ClientSocket);
		return 0;
	}

	int keepListening(SOCKET ListenSocket, Router &router) {
		int iResult;
		SOCKET ClientSocket = INVALID_SOCKET;

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closeSocket(ListenSocket);
			return 1;
		}
		while (true) {
			// Accept a client socket
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				printf("accept failed with error: %d\n", WSAGetLastError());
				closeSocket(ListenSocket);
				return 1;
			}
			onReceive(ClientSocket, router);

			// No longer need server socket
			closeSocket(ListenSocket);
			break;
		}
		return -1;
	}

	//F &routerFunction

	int winsockLib(Router &router) {
		cout << router.toString();
		WSADATA wsaData;
		int iResult;

		SOCKET ListenSocket = INVALID_SOCKET;

		struct addrinfo *result = NULL;
		struct addrinfo hints;


		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			return 1;
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the server address and port
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		// Create a SOCKET for the server to listen for client connections.
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Setup the TCP listening socket
		iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closeSocket(ListenSocket);
			return 1;
		}

		freeaddrinfo(result);

		do {
			iResult = keepListening(ListenSocket, router);
		} while (iResult != -1);

		return 0;
	}
}

int __cdecl main(void)
{
	mssc::Router router;
	router
		.push(*new mssc::Link("/flir-cam-trigger-set-on", [](char* msg, int len) {
			printf("From %s, Received DATA: %s", "/uri1", msg);
			// FLIR Trigger Set Procedure Code Location.
	}));
	return winsockLib(router);
}