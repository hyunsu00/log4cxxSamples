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
#include <cstring> // std::strerror
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define WSAEWOULDBLOCK  EWOULDBLOCK
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

	inline int Close(SOCKET socket, bool isShutdown = false)
	{
#ifdef _WIN32
		if (isShutdown) {
			shutdown(socket, SD_BOTH);
		}
		return closesocket(socket);
#else
		if (isShutdown) {
			shutdown(socket, SHUT_RDWR);
		}
		return close(socket);
#endif
	}

	inline int Read(SOCKET socket, char* buf, int len)
	{
#ifdef _WIN32
		return recv(socket, buf, len, 0);
#else
		return read(socket, buf, len);
#endif
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

	inline unsigned long getError()
	{
#ifdef _WIN32
		return WSAGetLastError();
#else
		return errno;
#endif
	}

	inline std::string getErrorText()
	{
#ifdef _WIN32
		char message[256] = { 0,  };
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, WSAGetLastError(), 0, message, 256, 0);
		char* nl = strrchr(message, '\n');
		if (nl) *nl = 0;
#else 
		return std::strerror(errno);
#endif
	}

}}} // log4cxx::ext::socket
