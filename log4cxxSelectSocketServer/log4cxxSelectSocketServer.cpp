// log4cxxSelectSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include <array>
#include <cassert>
#include <iostream>
#include <set>
#include <algorithm> // std::max

#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/loglog.h>		  // log4cxx::helpers::LogLog
#include "log4cxxSocket.h"
#include "log4cxxClient.h"

#ifdef _WIN32
#else
#	include <string.h>	// strdup
#	include <libgen.h>	// dirname
#endif

const char* const SERVER_LOGGER = "serverLogger";
inline log4cxx::LoggerPtr serverLogger()
{
    return log4cxx::Logger::getLogger(SERVER_LOGGER);
}
using LogLog = log4cxx::helpers::LogLog;

namespace {
    const int DEFAULT_PORT = 9988;
    const int DEFAULT_BUFFER_LEN = 8192;
} // namespace

//auto runServer = [](int port_num) -> void {
void runServer(int port_num) {
    auto maxSocketId = [](SOCKET serverSocket, const std::set<log4cxx::ext::socket::Client>& clientSockets) -> SOCKET {
        if (clientSockets.empty()) {
            return serverSocket;
        } else {
            return std::max<SOCKET>(serverSocket, (SOCKET)*clientSockets.rbegin());
        }
    };

    SOCKET serverSocket = log4cxx::ext::socket::Create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("소켓을 못열었다."));
        return;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0x00, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 소켓 주소 자동 할당
    serverAddr.sin_port = htons(port_num);			// 서버 포트 설정

    // 소켓 바인딩
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("바인딩 실패."));
        return;
    }

    int ret = log4cxx::ext::socket::setNonblock(serverSocket);
    if (ret == -1) {
        LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("논블로킹 소켓 설정 실패."));
        return;
    }

    // 소켓 리슨
    if (listen(serverSocket, 5) < 0) {
        LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("리슨 실패."));
        return;
    }
    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("포트 = ") << port_num);
    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

    log4cxx::ext::socket::Client::setLogger(SERVER_LOGGER);
    std::set<log4cxx::ext::socket::Client> clientSockets;

    fd_set fds;
    while (true) {
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);

        for (auto& sock: clientSockets) {
            FD_SET(sock, &fds);
        }

        SOCKET max = maxSocketId(serverSocket, clientSockets);
        int eventCount = select(static_cast<int>(max + 1), &fds, nullptr, nullptr, nullptr);
        if (eventCount <= 0) {
            // eventCount == 0 : 타임아웃
            // evnetCount < 0 : 함수 실패
            LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("select() failed : eventCount = ") << eventCount << LOG4CXX_STR(", select()함수가 실패하여 프로그램을 종료한다."));
            break;
        }
        LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("select() succeeded : eventCount = ") << eventCount);

        auto it = clientSockets.begin();
        while (it != clientSockets.end()) {
            SOCKET clientSocket = *it;
            if (FD_ISSET(clientSocket, &fds)) {
                //
                eventCount--;

                std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientSocket);
                std::array<char, DEFAULT_BUFFER_LEN> buf;
                //int recvBytes = recv(sock, buf.data(), kReadBufferSize, MSG_NOSIGNAL);
                int recvBytes = recv(clientSocket, buf.data(), DEFAULT_BUFFER_LEN, 0);
#if 0                
                if (recvBytes <= 0 && errno != EWOULDBLOCK) {
                    shutdown(clientSocket, SHUT_RDWR);
                    close(clientSocket);
                    it = clientSockets.erase(it);
                    //
                    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [clientCount = ") << clientSockets.size() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] read()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << errno << LOG4CXX_STR(")"));
                } else if (recvBytes > 0) {
                    ++it;
                    //
                    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
                } else {
                    ++it;
                    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)"));
                }
#else
                if (recvBytes < 0) { // 에러
					switch (errno)
					{
					case EWOULDBLOCK: // read 버퍼가 비어있음
                        ++it;
                        //
                        LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)"));
						break;
					default:
                        log4cxx::ext::socket::Close(clientSocket);
                        it = clientSockets.erase(it);
                        //
                        LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [clientCount = ") << clientSockets.size() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] read()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << errno << LOG4CXX_STR(")"));
					}
					break;
				} else if (recvBytes == 0) { // 클라이언트 접속 끊김
                    log4cxx::ext::socket::Close(clientSocket);
                    it = clientSockets.erase(it);
                    //
                    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [clientCount = ") << clientSockets.size() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
					break;
				} else { // 클라이언트 데이터 수신됨
                    (*it).forceLog(&buf[0], recvBytes);
                    ++it;
                    //
                    LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
				}
#endif
            } else {
                ++it;
            }
        } // while

        if (FD_ISSET(serverSocket, &fds)) {
            //
            eventCount--;

            struct sockaddr_in clientAddr;
            int len = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, (socklen_t*)&len);
            if (clientSocket < 0) {
                LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("accept 시도 실패."));
                break;
            }
            log4cxx::ext::socket::setNonblock(clientSocket);
            clientSockets.emplace(clientSocket);

            std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientAddr);
            LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 접속 - ") << clientInfo.c_str());
        }

        LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("select() unprocessed : eventCount = ") << eventCount);

    } // while
};

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    std::string exeDir;
#ifdef _WIN32	
    {
        char drive[_MAX_DRIVE] = { 0, }; // 드라이브 명
        char dir[_MAX_DIR] = { 0, }; // 디렉토리 경로
        _splitpath_s(argv[0], drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
        exeDir = std::string(drive) + dir;
    }
#else
    {
        char* exePath = strdup(argv[0]);
        exeDir = dirname(exePath);
        free(exePath);
        exeDir += "/";
    }
#endif
    std::string filePath = exeDir + "log4cxxSelectSocketServer.conf";
    log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

    log4cxx::ext::socket::Init();

    {
        runServer(DEFAULT_PORT);
    }

    log4cxx::ext::socket::Quit();

    return 0;
}