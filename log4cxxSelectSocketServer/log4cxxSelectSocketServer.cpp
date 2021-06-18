// log4cxxSelectSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include <array>
#include <cassert>
#include <iostream>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "ByteBufInputStream.h"
#include "ObjectLoader.h"
#include <string.h> // strdup
#include <libgen.h> // dirname
#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/exception.h>	  // log4cxx::helpers::InstantiationException, log4cxx::helpers::RuntimeException

namespace {

    const int kPort = 9988;
    const int kReadBufferSize = 8192;

} // namespace

class log4cxxClient
{
public:
    log4cxxClient(int socket)
    : m_Socket(socket)
    , m_Start(true)
    , m_ByteBuf()
    {
    }
    bool operator<(const log4cxxClient& other) const {
        return m_Socket < other.m_Socket;
    }
    operator int() const {
        return m_Socket;
    }

    bool forceLog(const std::array<char, kReadBufferSize>& byteBuf, size_t size) const
    {
        m_ByteBuf.insert(m_ByteBuf.end(), byteBuf.begin(), byteBuf.begin() + size);
        if (m_Start) {
            size_t readBytes = 0;
            try {
                readBytes = log4cxx::ext::loader::readStart(m_ByteBuf);
            } catch (log4cxx::ext::SmallBufferException& e) { // 무시
                // LOG4CXX_WARN(serverLogger(), e.what());
                return true;
            } catch (log4cxx::ext::InvalidBufferException& e) { // 종료
                // LOG4CXX_ERROR(serverLogger(), e.what());
                return false;
            }
            m_ByteBuf.erase(m_ByteBuf.begin(), m_ByteBuf.begin() + readBytes);

            m_Start = false;
        }

        if (!m_Start) {
            while (!m_ByteBuf.empty()) {
                try {
                    log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::createLoggingEvent(m_ByteBuf);
                    log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
                    if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
                        log4cxx::helpers::Pool p;
                        remoteLogger->callAppenders(event, p);
                    }
                } catch (log4cxx::ext::SmallBufferException& e) { // 무시
                    // LOG4CXX_WARN(serverLogger(), e.what());
                    break;
                } catch (log4cxx::ext::InvalidBufferException& e) { // 종료
                    // LOG4CXX_ERROR(serverLogger(), e.what());
                    return false;
                }
            }
        }

        return true;
    }

private:
    int m_Socket;
    mutable bool m_Start;
    mutable std::vector<char> m_ByteBuf;
}; // class log4cxxClient 


int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int maxSocketId(int serverSocket, const std::set<log4cxxClient>& clientSockets)
{
    if (clientSockets.empty()) {
        return serverSocket;
    } else {
        return std::max(serverSocket, (int)*clientSockets.rbegin());
    }
}

// returns server socket descriptor
int startListening(int port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1) {
        std::cerr << "cannot open socket\n";
        return -1;
    }

    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(serverSocket, (struct sockaddr *)(&sockAddr), sizeof(sockAddr));
    if (ret == -1) {
        std::cerr << "failed to bind socket\n";
        return -1;
    }

    ret = set_nonblock(serverSocket);
    if (ret == -1) {
        std::cerr << "failed to set non-blocking mode\n";
        return -1;
    }

    ret = listen(serverSocket, SOMAXCONN);
    if (ret == -1) {
        std::cerr << "failed to set non-blocking mode\n";
        return -1;
    }
    return serverSocket;
}

int main(int argc, char* argv[])
{
    {
        char* exePath = strdup(argv[0]);
        std::string exeDir = dirname(exePath);
        free(exePath);
        exeDir += "/";
        std::string filePath = exeDir + "log4cxxSelectSocketServer.conf";
        log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));
    }

    printf("run log4cxxSelectSocketServer\n");

    int serverSocket = startListening(kPort);
    if (serverSocket == -1) {
        std::cerr << "invalid socket\n";
        return -1;
    }

    std::set<log4cxxClient> clientSockets;

    fd_set fds;

    while (true) {
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);

        for (auto sock: clientSockets) {
            FD_SET(sock, &fds);
        }

        int max = maxSocketId(serverSocket, clientSockets);
        int eventCount = select(max + 1, &fds, nullptr, nullptr, nullptr);
        if (eventCount <= 0) {
            // eventCount == 0 : 타임아웃
            // evnetCount < 0 : 함수 실패
            printf("select() failed : eventCount = %d, select()함수가 실패하여 프로그램을 종료한다.\n", eventCount);
            break;
        }
        printf("select() : eventCount = %d\n", eventCount);

        auto it = clientSockets.begin();
        while (it != clientSockets.end()) {
            int sock = *it;
            if (FD_ISSET(sock, &fds)) {
                std::array<char, kReadBufferSize> buf;
                int nRead = recv(sock, buf.data(), kReadBufferSize, MSG_NOSIGNAL);
#if 0                
                if (nRead <= 0 && errno != EAGAIN) {
                    shutdown(sock, SHUT_RDWR);
                    close(sock);
                    it = clientSockets.erase(it);
                    //
                    printf("client_fd[%d], clientCount[%d] : resultBytes = %d, recv()함수가 실패하여 소켓을 종료한다.\n", sock, (int)clientSockets.size(), nRead);
                } else if (nRead > 0) {
                    ++it;
                    //
                    printf("client_fd[%d] : resultBytes = %d, 클라이언트 데이터 수신됨\n", sock, nRead);
                } else {
                    ++it;
                    printf("client_fd[%d] : resultBytes = %d, read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)\n", sock, nRead);
                }
                eventCount--;
#else
                if (nRead < 0) { // 에러
					switch (errno)
					{
					case EWOULDBLOCK: // read 버퍼가 비어있음
                        ++it;
                        //
                        printf("client_fd[%d] : resultBytes = %d, read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)\n", sock, nRead);
						break;
					default:
						close(sock);
                        it = clientSockets.erase(it);
                        //
                        printf("client_fd[%d], clientCount[%d] : resultBytes = %d, read()함수가 실패하여 소켓을 종료한다. (error = %d)\n", sock, (int)clientSockets.size(), nRead, errno);
					}
					break;
				} else if (nRead == 0) { // 클라이언트 접속 끊김
                    close(nRead);
                    it = clientSockets.erase(it);
                    //
                    printf("client_fd[%d], clientCount[%d] : resultBytes = %d, 클라이언트의 접속이 끊겨 소켓을 종료한다.\n", sock, (int)clientSockets.size(), nRead);
					break;
				} else { // 클라이언트 데이터 수신됨
                    (*it).forceLog(buf, nRead);
                    ++it;
                    //
                    printf("client_fd[%d] : resultBytes = %d, 클라이언트 데이터 수신됨\n", sock, nRead);
				}

                eventCount--;
#endif
            } else {
                ++it;
            }
        }
        if (FD_ISSET(serverSocket, &fds)) {
            int sock = accept(serverSocket, 0, 0);
            if (sock == -1) {
                std::cerr << "accept error\n";
                return -1;
            }
            set_nonblock(sock);
            clientSockets.insert(sock);
            //
            printf("Client Accept client_fd[%d], clientCount[%d]\n", sock, (int)clientSockets.size());
            eventCount--;
        }

        printf("처리되지 않은 eventCount = %d\n", eventCount);
    }

    return 0;
}