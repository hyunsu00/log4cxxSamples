// log4cxxPollSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include "BuildConfig.h"
#include <array>
#include <set>
#include <algorithm> // std::find_if

#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/loglog.h>		  // log4cxx::helpers::LogLog
#include "log4cxxSocket.h"
#include "log4cxxClient.h"

#ifdef _WIN32
#else
#include <sys/poll.h> // poll
#include <string.h> // strdup
#include <libgen.h> // dirname
#endif

const char* const SERVER_LOGGER = "serverLogger";
static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getRootLogger();
using LogLog = log4cxx::helpers::LogLog;

namespace {
	const int DEFAULT_PORT = 9988;
	const int DEFAULT_BUFFER_LEN = 8192;
} // namespace

auto runServer = [](int port_num) -> void {

	log4cxx::ext::socket::Client::setLogger(SERVER_LOGGER);

	std::vector<pollfd> pollfds;
	std::set<log4cxx::ext::socket::Client> clientSockets;

	auto createPoll = [](pollfd* fds, size_t nfds, int timeout) -> int {
#ifdef _WIN32
		return WSAPoll(fds, (ULONG)nfds, timeout);
#else
		return poll(fds, (nfds_t)nfds, timeout);
#endif
	};

	auto isReadSet = [](const pollfd& fd) -> bool {
#ifdef _WIN32
		if (fd.revents & POLLRDNORM || fd.revents & POLLHUP) {
#else
		if (fd.revents == POLLIN) {
#endif
			return true;
		}

		return false;
	};

	auto addPollfd = [&pollfds, &clientSockets](SOCKET socket) -> void {

		auto it = std::find_if(pollfds.begin(), pollfds.end(),
			[=](const pollfd& fdSet) -> bool {
				return fdSet.fd == socket;
			}
		);
		if (it == pollfds.end()) {
#ifdef _WIN32
			pollfd event = { socket, POLLRDNORM, 0 };
#else
			pollfd event = { socket, POLLIN, 0 };
#endif
			pollfds.push_back(event);
		}			

		clientSockets.emplace(socket);
	};

	auto delPollfd = [&pollfds, &clientSockets](SOCKET socket) -> void {
		auto it = std::find_if(pollfds.begin(), pollfds.end(),
			[=](const pollfd& fdSet) -> bool {
				return fdSet.fd == socket;
			}
		);
		if (it != pollfds.end()) {
			pollfds.erase(it);
		}

		log4cxx::ext::socket::Close(socket);
		clientSockets.erase(socket);
	};

	// 소켓 생성
	SOCKET serverSocket = log4cxx::ext::socket::Create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓을 못열었다."));
		return;
	}

	// 소켓 옵션 설정.
	// Option -> SO_REUSEADDR : 비정상 종료시 해당 포트 재사용 가능하도록 설정
	int option = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓옵션(SO_REUSEADDR) 실패."));
		goto CLEAN_UP;
	}

	// 논블록킹 소켓 설정
	if (log4cxx::ext::socket::setNonblock(serverSocket) < 0) {
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
		goto CLEAN_UP;
	}

	// 소켓 속성 설정
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port_num);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY : 사용가능한 랜카드 IP 사용

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
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("포트 = ") << port_num);
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

	addPollfd(serverSocket);

	while (true) {

		int eventCount = createPoll(&pollfds[0], pollfds.size(), -1);
		if (eventCount <= 0) {
			// event_count == 0 : 타임아웃
			// event_count < 0 : 함수 실패
			LOG4CXX_FATAL(sLogger, LOG4CXX_STR("poll() failed : eventCount = ") << eventCount << LOG4CXX_STR(", poll()함수가 실패하여 프로그램을 종료한다."));
			break;
		}
		LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("poll() succeeded : eventCount = ") << eventCount);

		auto it = pollfds.begin() + 1;
		while (it != pollfds.end()) {
			SOCKET clientSocket = it->fd;
			if (!isReadSet(*it)) {
				++it;
				continue;
			}
			//
			eventCount--;

			std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientSocket);
			std::array<char, DEFAULT_BUFFER_LEN> readBuf;
			int resultBytes = recv(clientSocket, readBuf.data(), DEFAULT_BUFFER_LEN, 0);
            if (resultBytes < 0) { // 에러
				switch (errno)
				{
				case EWOULDBLOCK: // read 버퍼가 비어있음
					++it;
					//
					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] recv()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)"));
					break;
				default:
					log4cxx::ext::socket::Close(clientSocket);
					clientSockets.erase(clientSocket);
					it = pollfds.erase(it);
					//
					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] recv()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << errno << LOG4CXX_STR(")"));
				}
			} else if (resultBytes == 0) { // 클라이언트 접속 끊김
				log4cxx::ext::socket::Close(clientSocket);
				clientSockets.erase(clientSocket);
				it = pollfds.erase(it);
				//
				LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
			} else { // 클라이언트 데이터 수신됨
				auto itClient = clientSockets.find(it->fd);
				LOG4CXX_ASSERT(sLogger, itClient != clientSockets.end(), LOG4CXX_STR("clientSockets.find() != clientSockets.end()"));
				if (itClient != clientSockets.end()) {
					if (!itClient->forceLog(&readBuf[0], resultBytes)) {
						log4cxx::ext::socket::Close(clientSocket);
						clientSockets.erase(clientSocket);
						it = pollfds.erase(it);
					} else {
						++it;
					}
				} else {
					++it;
				}
				//
				LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
			} // if
		} // while

		if (isReadSet(pollfds[0])) {
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
				log4cxx::ext::socket::Close(clientSocket);
				LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
				continue;
			}

			addPollfd(clientSocket);
			std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientAddr);
			LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 접속 - ") << clientInfo.c_str());
		}

		LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("poll() unprocessed : eventCount = ") << eventCount);

	} // while

CLEAN_UP:
	for (auto& clientSocket : clientSockets) {
		log4cxx::ext::socket::Close(clientSocket);
	}
	log4cxx::ext::socket::Close(serverSocket);
};

int main(int argc, char *argv[])
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
	std::string filePath = exeDir + "log4cxxPollSocketServer.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));
	sLogger = log4cxx::Logger::getLogger(SERVER_LOGGER);

	log4cxx::ext::socket::Init();

	{
		runServer(DEFAULT_PORT);
	}
	
	log4cxx::ext::socket::Quit();

	return 0;
}
