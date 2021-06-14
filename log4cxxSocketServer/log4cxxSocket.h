// log4cxxSocket.h
#pragma once

#ifdef _WIN32
  /* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
  /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#endif

namespace log4cxx { namespace ext { namespace socket {

	inline int Init(void)
	{
#ifdef _WIN32
		WSADATA wsa_data;
		return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
		return 0;
#endif
	}

	inline int Quit(void)
	{
#ifdef _WIN32
		return WSACleanup();
#else
		return 0;
#endif
	}

	inline SOCKET Create(int af, int type, int protocol)
	{
		return ::socket(af, type, protocol);
	}

	inline int Close(SOCKET socket)
	{
		int status = 0;
#ifdef _WIN32
		status = shutdown(socket, SD_BOTH);
		if (status == 0) { status = closesocket(socket); }
#else
		status = shutdown(socket, SHUT_RDWR);
		if (status == 0) { status = close(socket); }
#endif
		return status;
	}

	inline std::string getClientInfo(const struct sockaddr_in& clientAddr)
	{
		char clientInfo[20] = { 0, };
#ifdef _WIN32		
		inet_ntop(AF_INET, &clientAddr.sin_addr.S_un, clientInfo, sizeof(clientInfo));
#else
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientInfo, sizeof(clientInfo));
#endif
		return clientInfo;
	}

}}} // log4cxx::ext::socket
