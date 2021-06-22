// log4cxxPollSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include "BuildConfig.h"
#include <array>
#include <set>

#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <sys/ioctl.h> // ioctl
#include <string.h> // strdup
#include <libgen.h> // dirname

#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/loglog.h>		  // log4cxx::helpers::LogLog
#include "log4cxxSocket.h"
#include "log4cxxClient.h"


#define LISTEN_BACKLOG 15 // 일반적인 connection의 setup은 client가 connect()를 사용하여 connection request를 전송하면 server는 accept()를 사용하여 connection을 받아들입니다.
                          // 그런데 만약 server가 다른 작업을 진행 중이면 accept()를 할 수 없는 경우가 발생하고 이 경우 connection request는 queue에서 대기하는데 backlog는
                          // 이와 같이 accept() 없이 대기 가능한 connection request의 최대 개수입니다.
                          // 보통 5정도의 value를 사용하며 만약 아주 큰 값을 사용하면 kernel의 resource를 소모합니다.
                          // 따라서, 접속 가능한 클라이언트의 최대수가 아닌 연결을 기다리는 클라이언트의 최대수입니다

const char* const SERVER_LOGGER = "serverLogger";
static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getRootLogger();
using LogLog = log4cxx::helpers::LogLog;

namespace {
	const int DEFAULT_PORT = 9988;
	const int DEFAULT_BUFFER_LEN = 8192;
	const int NUM_FDS = 5;
} // namespace

auto runServer = [](int port_num) -> void {

	log4cxx::ext::socket::Client::setLogger(SERVER_LOGGER);
	std::set<log4cxx::ext::socket::Client> clientSockets;

	struct pollfd pollfds[NUM_FDS] = { 0, };
	int pollnum = 0;

	auto pollfds_add = [&pollnum, &pollfds](int fd) -> int {
		int i = 0;
		if (pollnum < NUM_FDS) {
			i = pollnum++;
		} else {
			for (i = 0; i < NUM_FDS; i++) {
				if (pollfds[i].fd < 0) {
					goto found;
				}
			}

			return -1;
		}

	found:
		pollfds[i].fd = fd;
		pollfds[i].events = POLLIN;

		return 0;
	};

	auto pollfds_del = [&pollnum, &pollfds, &clientSockets](int fd) -> void {
		for (int i = 0; i < pollnum; i++) {
			if (pollfds[i].fd == fd) {
				pollfds[i].fd = -1;
				break;
			}
		}
		close(fd);
		clientSockets.erase(fd);
	};

	// 소켓 생성
	int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0) {
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓을 못열었다."));
		return;
	}

	// server fd Non-Blocking Socket으로 설정. Edge Trigger 사용하기 위해 설정.
	int flags = fcntl(server_fd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(server_fd, F_SETFL, flags) < 0) {
		close(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
		return;
	}

	// 소켓 옵션 설정.
	// Option -> SO_REUSEADDR : 비정상 종료시 해당 포트 재사용 가능하도록 설정
	int option = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
		close(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓옵션(SO_REUSEADDR) 실패."));
		return;
	}

	// 소켓 속성 설정
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY : 사용가능한 랜카드 IP 사용

	// 소켓 속성과 소켓 fd 연결
	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		close(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("바인딩 실패."));
		return;
	}

	// 응답 대기
	if (listen(server_fd, LISTEN_BACKLOG) < 0) {
		close(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("리슨 실패."));
		return;
	}
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("포트 = ") << port_num);
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

	pollfds_add(server_fd);

	int event_count = 0;
	while (true) {
		event_count = poll(pollfds, pollnum, -1);

		if (event_count <= 0) {
			// event_count == 0 : 타임아웃
			// event_count < 0 : 함수 실패
			LOG4CXX_FATAL(sLogger, LOG4CXX_STR("poll() failed : event_count = ") << event_count << LOG4CXX_STR(", poll()함수가 실패하여 프로그램을 종료한다."));
			goto CLEAN_UP;
		}

		for (int i = 0; i < pollnum; i++) {

			if (pollfds[i].revents == 0) {
				continue;
			}
			if (pollfds[i].revents != POLLIN) {
				continue;
			}

			if (pollfds[i].fd == server_fd) {
				event_count--;

				struct sockaddr_in clientAddr;
				int len = sizeof(clientAddr);
				SOCKET client_fd = accept(pollfds[i].fd, (struct sockaddr*)&clientAddr, (socklen_t*)&len);
				if (client_fd < 0) {
					LOG4CXX_FATAL(sLogger, LOG4CXX_STR("accept 시도 실패."));
					continue;
				}
				// 논블록킹 소켓 설정
				if (log4cxx::ext::socket::setNonblock(client_fd) < 0) {
					log4cxx::ext::socket::Close(client_fd);
					LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
					continue;
				}

				if (pollfds_add(client_fd) < 0) {
					log4cxx::ext::socket::Close(client_fd);
					LOG4CXX_FATAL(sLogger, LOG4CXX_STR("클라이언트 최대 연결을 초과하여 종료한다."));
					continue;
				}

				clientSockets.emplace(client_fd);
				std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientAddr);
				LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 접속 - ") << clientInfo.c_str());
			} else {
				event_count--;

				int client_fd = pollfds[i].fd;
				std::string client_info = log4cxx::ext::socket::getClientInfo(client_fd);

				std::array<char, DEFAULT_BUFFER_LEN> readBuf;
				int resultBytes = read(client_fd, readBuf.data(), DEFAULT_BUFFER_LEN);
                if (resultBytes < 0) { // 에러
					switch (errno)
					{
					case EWOULDBLOCK: // read 버퍼가 비어있음
						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)"));
						break;
					default:
						pollfds_del(client_fd);

						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << errno << LOG4CXX_STR(")"));
					}
				} else if (resultBytes == 0) { // 클라이언트 접속 끊김
					pollfds_del(client_fd);

					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
				} else { // 클라이언트 데이터 수신됨
					auto it = clientSockets.find(client_fd);
					LOG4CXX_ASSERT(sLogger, it != clientSockets.end(), LOG4CXX_STR("clientSockets.find() != clientSockets.end()"));
					if (it != clientSockets.end()) {
						it->forceLog(&readBuf[0], resultBytes);
					}

					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
				} // if
			} // if
		} // for
	} // while

CLEAN_UP:
	for (auto& clientSocket : clientSockets) {
		log4cxx::ext::socket::Close(clientSocket);
	}
	close(server_fd);
};


int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	std::string exeDir;
	{
		char* exePath = strdup(argv[0]);
		exeDir = dirname(exePath);
		free(exePath);
		exeDir += "/";
	}
	std::string filePath = exeDir + "log4cxxPollSocketServer.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));
	sLogger = log4cxx::Logger::getLogger(SERVER_LOGGER);

	{
		runServer(DEFAULT_PORT);
	}
	
	return 0;
}
