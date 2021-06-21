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
#include <fcntl.h> /* fcntl */
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
		//shutdown(socket, SD_BOTH);
		status = closesocket(socket);
#else
		//shutdown(sock, SHUT_RDWR);
		status = close(socket);
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

	inline std::string getClientInfo(SOCKET socket)
	{
		struct sockaddr_in clientAddr;
#ifdef _WIN32
		int len = sizeof(clientAddr);
#else
		socklen_t len = sizeof(clientAddr);
#endif
		if (getsockname(socket, (struct sockaddr*)&clientAddr, &len) < 0) {
			return std::string();
		}

		return getClientInfo(clientAddr);
	}

	inline int setNonblock(SOCKET socket)
	{
#ifdef _WIN32
		u_long flags = 1;
		return ioctlsocket(socket, FIONBIO, &flags);
#else
		int flags;
		if (-1 == (flags = fcntl(socket, F_GETFL, 0))) {
			flags = 0;
		}
		return fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
	}

}}} // log4cxx::ext::socket
