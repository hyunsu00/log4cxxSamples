// log4cxxEpollSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include "BuildConfig.h"
#include <array>
#include <set>

#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/loglog.h>		  // log4cxx::helpers::LogLog
#include "log4cxxSocket.h"
#include "log4cxxClient.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#include "wepoll.h"
int (*close)(HANDLE) = epoll_close;
#else
#include <sys/epoll.h> // epoll_create, epoll_wait, epoll_ctl
#include <string.h> // strdup
#include <libgen.h> // dirname
typedef int HANDLE;
#endif

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
} // namespace

auto runServer = [](int port_num) -> void {

	log4cxx::ext::socket::Client::setLogger(SERVER_LOGGER);
	std::set<log4cxx::ext::socket::Client> clientSockets;

	auto epoll_ctl_del = [&clientSockets](HANDLE epoll_fd, SOCKET client_fd) {
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
		closesocket(client_fd);
		clientSockets.erase(client_fd);
	};

	// 소켓 생성
	SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0) {
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓을 못열었다."));
		return;
	}

	// server fd Non-Blocking Socket으로 설정. Edge Trigger 사용하기 위해 설정.
	if (log4cxx::ext::socket::setNonblock(server_fd) < 0) {
		closesocket(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
		return;
	}

	// 소켓 옵션 설정.
	// Option -> SO_REUSEADDR : 비정상 종료시 해당 포트 재사용 가능하도록 설정
	int option = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
		closesocket(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓옵션(SO_REUSEADDR) 실패."));
		return;
	}

	// Nagle 알고리즘 끄기
	if (setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option)) < 0) {
		closesocket(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("Nagle 알고리즘 OFF(TCP_NODELAY) 실패."));
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
		closesocket(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("바인딩 실패."));
		return;
	}

	// 응답 대기
	if (listen(server_fd, LISTEN_BACKLOG) < 0) {
		closesocket(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("리슨 실패."));
		return;
	}
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("포트 = ") << port_num);
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

	// Epoll fd 생성
	HANDLE epoll_fd = epoll_create(1024); // size 만큼의 커널 폴링 공간을 만드는 함수
	if (epoll_fd < 0) {
		closesocket(server_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("epoll_create() 함수 실패"));
		return;
	}

	// server fd, epoll에 등록
	// EPOLLET => Edge Trigger 사용
	// 레벨트리거와 에지 트리거를 소켓 버퍼에 대응하면, 소켓 버퍼의 레벨 즉 데이터의 존재 유무를 가지고 판단하는 것이
	// 레벨트리거 입니다.즉 읽어서 해당 처리를 하였다 하더라도 덜 읽었으면 계속 이벤트가 발생하겠지요.예를 들어
	// 1000바이트가 도착했는데 600바이트만 읽었다면 레벨 트리거에서는 계속 이벤트를 발생시킵니다.그러나
	// 에지트리거에서는 600바이트만 읽었다 하더라도 더 이상 이벤트가 발생하지 않습니다.왜냐하면 읽은 시점을 기준으로
	// 보면 더이상의 상태 변화가 없기 때문이죠..LT 또는 ET는 쉽게 옵션으로 설정 가능합니다.
	// 참고로 select / poll은 레벨트리거만 지원합니다.
	struct epoll_event server_event;
#if defined(EDGE_TRIGGER) && !defined(_WIN32) 
	// 에지 트리거(Edge Trigger, ET)
	server_event.events = EPOLLIN | EPOLLET;
#else
	// 레벨 트리거(Level Trigger, LT)
	server_event.events = EPOLLIN;
#endif
	server_event.data.fd = server_fd;

	/* server events set(read for accept) */
	// epoll_ctl : epoll이 관심을 가져주기 바라는 FD와 그 FD에서 발생하는 event를 등록하는 인터페이스.
	// EPOLL_CTL_ADD : 관심있는 파일디스크립트 추가
	// EPOLL_CTL_MOD : 기존 파일 디스크립터를 수정
	// EPOLL_CTL_DEL : 기존 파일 디스크립터를 관심 목록에서 삭제
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event) < 0) {
		closesocket(server_fd);
		close(epoll_fd);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("epoll_ctl() 함수 실패"));
		return;
	}

	// epoll wait.
	// 관심있는 fd들에 무슨일이 일어났는지 조사.
	// 사건들의 리스트를 epoll_events[]의 배열로 전달.
	// 리턴값은 사건들의 개수, 사건의 최대 개수는 MAX_EVENTS 만큼
	// timeout  -1      --> 영원히 사건 기다림(blocking)
	//           0      --> 사건 기다리지 않음.
	//           0 < n  --> (n)ms 만큼 대기
	const int MAX_EVENTS = 1024;
	struct epoll_event epoll_events[MAX_EVENTS];
	int event_count;
	int timeout = -1;

	while (true) {

		event_count = epoll_wait(epoll_fd, epoll_events, MAX_EVENTS, timeout);
		if (event_count < 0) {
			if (errno == EINTR) { // 신호처리기(gdb)에 의해 중단됨
				continue;
			}
			LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("epoll_wait() failed : event_count = ") << event_count);
			break;
		}
		LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("[BEGIN] epoll_wait() succeeded : event_count = ") << event_count);

		for (int i = 0; i < event_count; i++) {
			// Accept
			if (epoll_events[i].data.fd == server_fd) {

				struct sockaddr_in client_addr;
				int client_len = sizeof(client_addr);
				SOCKET client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
				if (client_fd < 0) {
					LOG4CXX_FATAL(sLogger, LOG4CXX_STR("accept 시도 실패."));
					continue;
				}

				// client fd Non-Blocking Socket으로 설정. Edge Trigger 사용하기 위해 설정.
				if (log4cxx::ext::socket::setNonblock(client_fd) < 0) {
					closesocket(client_fd);
					LOG4CXX_FATAL(sLogger, LOG4CXX_STR("논블로킹 소켓 설정 실패."));
					continue;
				}

				// 클라이언트 fd, epoll 에 등록
				struct epoll_event client_event;
#if defined(EDGE_TRIGGER) && !defined(_WIN32) 
				// 에지 트리거(Edge Trigger, ET)
				client_event.events = EPOLLIN | EPOLLET;
#else
				// 레벨 트리거(Level Trigger, LT)
				client_event.events = EPOLLIN;
#endif
				client_event.data.fd = client_fd;

				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0) {
					closesocket(client_fd);
					LOG4CXX_FATAL(sLogger, LOG4CXX_STR("epoll_ctl() 함수 실패"));
					continue;
				}
				clientSockets.emplace(client_fd);

				std::string client_info = log4cxx::ext::socket::getClientInfo(client_addr);
				LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 접속 - ") << client_info.c_str());

			} else {
				// epoll에 등록 된 클라이언트들의 send data 처리
				int client_fd = epoll_events[i].data.fd;

				std::string client_info = log4cxx::ext::socket::getClientInfo(client_fd);

#ifdef EDGE_TRIGGER 
				// 엣지 트리거 모드
	#if 1
				u_long readBytes = 0;
				int ioResult = log4cxx::ext::socket::getTotalReadBytes(client_fd, readBytes);
				if (ioResult < 0) { // 에러
					epoll_ctl_del(epoll_fd, client_fd);

					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [readBytes = ") << readBytes << LOG4CXX_STR("] , [ioResult = ") << ioResult << LOG4CXX_STR(" ] ioctl()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << log4cxx::ext::socket::getError() << LOG4CXX_STR(")"));
				} else { // 성공
					LOG4CXX_ASSERT(sLogger, ioResult == 0, LOG4CXX_STR("성공시 ioResult > 0 이 들어오면 안된다."));

					readBytes = std::max<int>(readBytes, 1);
					std::vector<char> readBuf(readBytes, 0);
					int resultBytes = recv(client_fd, &readBuf[0], static_cast<int>(readBuf.size()), 0);
					if (resultBytes < 0) { // 에러
						switch (log4cxx::ext::socket::getError())
						{
						case WSAEWOULDBLOCK: // read 버퍼가 비어있음
							LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수의 버퍼는 비어있다. (errno = WSAEWOULDBLOCK(EWOULDBLOCK == EAGAIN))"));
							break;
						default:
							{
								epoll_ctl_del(epoll_fd, client_fd);

								LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR("] read()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << log4cxx::ext::socket::getError() << LOG4CXX_STR(")"));
							}
						}
					} else if (resultBytes == 0) { // 클라이언트 접속 끊김
						epoll_ctl_del(epoll_fd, client_fd);

						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
					} else { // 클라이언트 데이터 수신됨
						LOG4CXX_ASSERT(sLogger, resultBytes == readBytes, LOG4CXX_STR("소켓버퍼의 사이즈 만큼 읽어와야 한다."));
						auto it = clientSockets.find(client_fd);
						LOG4CXX_ASSERT(sLogger, it != clientSockets.end(), LOG4CXX_STR("clientSockets.find() != clientSockets.end()"));
						if (it != clientSockets.end()) {
							it->forceLog(&readBuf[0], resultBytes);
						}

						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
					}
				} // if		
	#else
				std::array<char, DEFAULT_BUFFER_LEN> readBuf;
				while (true) {
					int resultBytes = recv(client_fd, readBuf.data(), DEFAULT_BUFFER_LEN, 0);
					if (resultBytes < 0) { // 에러
						switch (log4cxx::ext::socket::getError())
						{
						case WSAEWOULDBLOCK: // read 버퍼가 비어있음
							LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수의 버퍼는 비어있다. (errno = WSAEWOULDBLOCK(EWOULDBLOCK == EAGAIN))"));
							break;
						default:
							{
								epoll_ctl_del(epoll_fd, client_fd);

								LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << log4cxx::ext::socket::getError() << LOG4CXX_STR(")"));
							}
						}
						break;
					} else if (resultBytes == 0) { // 클라이언트 접속 끊김
						epoll_ctl_del(epoll_fd, client_fd);

						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
						break;
					} else { // 클라이언트 데이터 수신됨
						auto it = clientSockets.find(client_fd);
						LOG4CXX_ASSERT(sLogger, it != clientSockets.end(), LOG4CXX_STR("clientSockets.find() != clientSockets.end()"));
						if (it != clientSockets.end()) {
							it->forceLog(&readBuf[0], resultBytes);
						}

						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
					} // if
				} // while
	#endif
#else
				// 레벨 트리거 코드
				std::array<char, DEFAULT_BUFFER_LEN> readBuf;
				int resultBytes = recv(client_fd, readBuf.data(), DEFAULT_BUFFER_LEN, 0);
				if (resultBytes < 0) { // 에러
					switch (log4cxx::ext::socket::getError())
					{
					case WSAEWOULDBLOCK: // read 버퍼가 비어있음
						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수의 버퍼는 비어있다. (errno = WSAEWOULDBLOCK(EWOULDBLOCK == EAGAIN))"));
						break;
					default:
						epoll_ctl_del(epoll_fd, client_fd);

						LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] read()함수가 실패하여 소켓을 종료한다. ") << LOG4CXX_STR("(error = ") << log4cxx::ext::socket::getError() << LOG4CXX_STR(")"));
					}
				} else if (resultBytes == 0) { // 클라이언트 접속 끊김
					epoll_ctl_del(epoll_fd, client_fd);

					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트의 접속이 끊겨 소켓을 종료한다."));
				} else { // 클라이언트 데이터 수신됨
					auto it = clientSockets.find(client_fd);
					LOG4CXX_ASSERT(sLogger, it != clientSockets.end(), LOG4CXX_STR("clientSockets.find() != clientSockets.end()"));
					if (it != clientSockets.end()) {
						it->forceLog(&readBuf[0], resultBytes);
					}

					LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << client_info.c_str() << LOG4CXX_STR("] , [resultBytes = ") << resultBytes << LOG4CXX_STR(" ] 클라이언트 데이터 수신됨"));
				} // if
#endif
			} // if
		} // for

		LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("[END  ] epoll_wait() unprocessed : eventCount = ") << 0);

	} // while

	// 리소스 해제
	for (auto& clientSocket : clientSockets) {
		closesocket(clientSocket);
	}
	closesocket(server_fd);
	close(epoll_fd);
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
	std::string filePath = exeDir + "log4cxxEpollSocketServer.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));
	sLogger = log4cxx::Logger::getLogger(SERVER_LOGGER);

	{
		runServer(DEFAULT_PORT);
	}
	
	return 0;
}
