// log4cxxSocket.h
#pragma once

#ifdef _WIN32
  /* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
  /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h> // inet_ntop
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <fcntl.h> /* fcntl */
#include <sys/ioctl.h> // ioctl
#include <netinet/tcp.h> // TCP_NODELAY
#include <cstring> // std::strerror
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SD_BOTH         SHUT_RDWR
#define WSAEWOULDBLOCK  EWOULDBLOCK
auto closesocket = [](int fd) -> int {
	return close(fd);
};
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

	inline int getTotalReadBytes(SOCKET socket, u_long& readBytes)
	{
#ifdef _WIN32
		return ioctlsocket(socket, FIONREAD, &readBytes);
#else
		return ioctl(socket, FIONREAD, &readBytes);
#endif
	}

	// Nagle 알고리즘 끄기 / 켜기
	inline int setTcpNodelay(SOCKET socket, int optval = 1)
	{
		return setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&optval, sizeof(optval));
	}

	// Nagle 알고리즘 반환
	inline int getTcpNodelay(SOCKET socket, int& optval)
	{
		socklen_t optlen = sizeof(int);
		return getsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, &optlen);
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
