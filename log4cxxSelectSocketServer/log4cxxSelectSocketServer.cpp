// log4cxxSelectSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include "BuildConfig.h"
#include <array> // std::array
#include <set> // std::set

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
static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getLogger(SERVER_LOGGER);
using LogLog = log4cxx::helpers::LogLog;

namespace {
    const int DEFAULT_PORT = 9988;
    const int DEFAULT_BUFFER_LEN = 8192;
} // namespace

// https://sourceware.org/bugzilla/show_bug.cgi?id=17082
// auto runServer = [](int port) -> void {
static void runServer(int port) {

    auto getMaxId = [=](SOCKET serverSocket, const std::set<log4cxx::ext::socket::Client>& clientSockets) -> int {
        if (clientSockets.empty()) {
            return static_cast<int>(serverSocket);
        } else {
            SOCKET maxSocketId = std::max<SOCKET>(serverSocket, (SOCKET)*clientSockets.rbegin());
            return static_cast<int>(maxSocketId);
        }
    };

    log4cxx::ext::socket::Client::setLogger(SERVER_LOGGER);
    std::set<log4cxx::ext::socket::Client> clientSockets;

    // 소켓 생성
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓을 못열었다."));
        return;
    }

    // 비정상 종료시 해당 포트 재사용 가능하도록 설정
    if (log4cxx::ext::socket::setReuseAddr(serverSocket) < 0) {
        LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓옵션(SO_REUSEADDR) 실패."));
        goto CLEAN_UP;
    }

    // Nagle 알고리즘 끄기
    // accept된 clientSocket도 Nagle 알고리즘 상속됨
    if (log4cxx::ext::socket::setTcpNodelay(serverSocket) < 0) {
        LOG4CXX_FATAL(sLogger, LOG4CXX_STR("Nagle 알고리즘 OFF(TCP_NODELAY) 실패."));
        goto CLEAN_UP;
    }

    // 논블록킹 소켓 설정
    if (log4cxx::ext::socket::setNonblock(serverSocket) < 0) {
        LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
        goto CLEAN_UP;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0x00, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 소켓 주소 자동 할당
    serverAddr.sin_port = htons(port); // 서버 포트 설정

    // 소켓 바인딩
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG4CXX_FATAL(sLogger, LOG4CXX_STR("바인딩 실패."));
        goto CLEAN_UP;
    }

    // 소켓 리슨
    if (listen(serverSocket, SOMAXCONN) < 0) {
        LOG4CXX_FATAL(sLogger, LOG4CXX_STR("리슨 실패."));
        goto CLEAN_UP;
    }
    LOG4CXX_INFO(sLogger, LOG4CXX_STR("포트 = ") << port);
    LOG4CXX_INFO(sLogger, LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

    fd_set fds;
    while (true) {
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);

        for (auto& sock: clientSockets) {
            FD_SET(sock, &fds);
        }

        int maxId = getMaxId(serverSocket, clientSockets);
        // 윈도우의 select 함수는 socket만 적용됨, stdin은 적용안됨
        int eventCount = select(maxId + 1, &fds, nullptr, nullptr, nullptr);
        if (eventCount <= 0) {
            // eventCount == 0 : 타임아웃
            // evnetCount < 0 : 함수 실패
            LOG4CXX_FATAL(sLogger, LOG4CXX_STR("select() failed : eventCount = ") << eventCount << LOG4CXX_STR(", select()함수가 실패하여 프로그램을 종료한다."));
            break;
        }
        LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("[BEGIN] select() succeeded : eventCount = ") << eventCount);

        auto it = clientSockets.begin();
        while (it != clientSockets.end()) {
            SOCKET clientSocket = *it;
            if (!FD_ISSET(clientSocket, &fds)) {
                ++it;
                continue;
            }
            //
            eventCount--;

            std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientSocket);
            std::array<char, DEFAULT_BUFFER_LEN> readBuf;
            //int resultBytes = recv(sock, readBuf.data(), DEFAULT_BUFFER_LEN, MSG_NOSIGNAL);
            int resultBytes = recv(clientSocket, readBuf.data(), DEFAULT_BUFFER_LEN, 0);
            if (resultBytes < 0) { // 에러
				switch (log4cxx::ext::socket::getError())
				{
				case WSAEWOULDBLOCK: // read 버퍼가 비어있음
                    ++it;
                    //
                    LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] recv()함수의 버퍼는 비어있다. (errno = WSAEWOULDBLOCK(EWOULDBLOCK == EAGAIN))"));
					break;
				default:
                    closesocket(clientSocket);
                    it = clientSockets.erase(it);
                    //
                    LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] recv()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << log4cxx::ext::socket::getError() << LOG4CXX_STR(")"));
				}
			} else if (resultBytes == 0) { // 클라이언트 접속 끊김
                closesocket(clientSocket);
                it = clientSockets.erase(it);
                //
                LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
			} else { // 클라이언트 데이터 수신됨
                bool result = (*it).forceLog(&readBuf[0], resultBytes);
                LOG4CXX_ASSERT(sLogger, result, LOG4CXX_STR("(*it).forceLog() Failed"));
                if (!result) {
                    closesocket(clientSocket);
                    it = clientSockets.erase(it);
                } else {
                    ++it;
                }
                //
                LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
			}
        } // while

        if (FD_ISSET(serverSocket, &fds)) {
            //
            eventCount--;

            struct sockaddr_in clientAddr;
            int len = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, (socklen_t*)&len);
            if (clientSocket < 0) {
                LOG4CXX_FATAL(sLogger, LOG4CXX_STR("accept 시도 실패."));
                continue;
            }
            // 논블록킹 소켓 설정
            if (log4cxx::ext::socket::setNonblock(clientSocket) < 0) {
                closesocket(clientSocket);
                LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
                continue;
            }

            clientSockets.emplace(clientSocket);
            std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientAddr);
            LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 접속 - ") << clientInfo.c_str());
        }

        LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("[END  ] select() unprocessed : eventCount = ") << eventCount);

    } // while

CLEAN_UP:
    for (auto& clientSocket : clientSockets) {
        closesocket(clientSocket);
    }
    closesocket(serverSocket);
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
    sLogger = log4cxx::Logger::getLogger(SERVER_LOGGER);

    log4cxx::ext::socket::Init();

    {
        runServer(DEFAULT_PORT);
    }

    log4cxx::ext::socket::Quit();

    return 0;
}