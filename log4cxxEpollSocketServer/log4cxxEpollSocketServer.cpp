﻿// log4cxxEpollSocketServer.cpp : 애플리케이션의 진입점을 정의합니다.
//

#include <cstdio>
#include <iostream>
#include <string.h>
#include <fcntl.h>

#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> // ioctl
#include <assert.h> // assert

#include "ObjectLoader.h"
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

//#define EDGE_TRIGGER	// 에지 트리거 정의
int main(int argc, char *argv[])
{
	char *exePath = strdup(argv[0]);
	std::string exeDir = dirname(exePath);
	free(exePath);
	exeDir += "/";
	std::string filePath = exeDir + "log4cxxEpollSocketServer.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

	printf("run log4cxxEpollSocketServer\n");
	int error_check;

	// 소켓 생성
	int server_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		printf("socket() error\n");
		return 0;
	}

	// server fd Non-Blocking Socket으로 설정. Edge Trigger 사용하기 위해 설정.
	int flags = fcntl(server_fd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(server_fd, F_SETFL, flags) < 0) {
		printf("server_fd fcntl() error\n");
		return 0;
	}

	// 소켓 옵션 설정.
	// Option -> SO_REUSEADDR : 비정상 종료시 해당 포트 재사용 가능하도록 설정
	int option = true;
	error_check = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if (error_check < 0) {
		printf("setsockopt() error[%d]\n", error_check);
		close(server_fd);
		return 0;
	}

	// 소켓 속성 설정
	struct sockaddr_in mSockAddr;
	memset(&mSockAddr, 0, sizeof(mSockAddr));
	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_port = htons(9988);
	mSockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY : 사용가능한 랜카드 IP 사용

	// 소켓 속성과 소켓 fd 연결
	error_check = bind(server_fd, (struct sockaddr *)&mSockAddr, sizeof(mSockAddr));
	if (error_check < 0) {
		printf("bind() error[%d]\n", error_check);
		close(server_fd);
		return 0;
	}

	// 응답 대기
	if (listen(server_fd, LISTEN_BACKLOG) < 0) {
		printf("listen() error\n");
		close(server_fd);
		return 0;
	}

	// Epoll fd 생성
	int epoll_fd = epoll_create(1024); // size 만큼의 커널 폴링 공간을 만드는 함수
	if (epoll_fd < 0) {
		printf("epoll_create() error\n");
		close(server_fd);
		return 0;
	}

	// server fd, epoll에 등록
	// EPOLLET => Edge Trigger 사용
	// 레벨트리거와 에지 트리거를 소켓 버퍼에 대응하면, 소켓 버퍼의 레벨 즉 데이터의 존재 유무를 가지고 판단하는 것이
	// 레벨트리거 입니다.즉 읽어서 해당 처리를 하였다 하더라도 덜 읽었으면 계속 이벤트가 발생하겠지요.예를 들어
	// 1000바이트가 도착했는데 600바이트만 읽었다면 레벨 트리거에서는 계속 이벤트를 발생시킵니다.그러나
	// 에지트리거에서는 600바이트만 읽었다 하더라도 더 이상 이벤트가 발생하지 않습니다.왜냐하면 읽은 시점을 기준으로
	// 보면 더이상의 상태 변화가 없기 때문이죠..LT 또는 ET는 쉽게 옵션으로 설정 가능합니다.
	// 참고로 select / poll은 레벨트리거만 지원합니다.
	struct epoll_event events;
#ifdef EDGE_TRIGGER 
	// 에지 트리거(Edge Trigger, ET)
	events.events = EPOLLIN | EPOLLET;
#else
	// 레벨 트리거(Level Trigger, LT)
	events.events = EPOLLIN;
#endif
	events.data.fd = server_fd;

	/* server events set(read for accept) */
	// epoll_ctl : epoll이 관심을 가져주기 바라는 FD와 그 FD에서 발생하는 event를 등록하는 인터페이스.
	// EPOLL_CTL_ADD : 관심있는 파일디스크립트 추가
	// EPOLL_CTL_MOD : 기존 파일 디스크립터를 수정
	// EPOLL_CTL_DEL : 기존 파일 디스크립터를 관심 목록에서 삭제
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &events) < 0) {
		printf("epoll_ctl() error\n");
		close(server_fd);
		close(epoll_fd);
		return 0;
	}

	// epoll wait.
	// 관심있는 fd들에 무슨일이 일어났는지 조사.
	// 사건들의 리스트를 epoll_events[]의 배열로 전달.
	// 리턴값은 사건들의 개수, 사건의 최대 개수는 MAX_EVENTS 만큼
	// timeout  -1      --> 영원히 사건 기다림(blocking)
	//           0      --> 사건 기다리지 않음.
	//           0 < n  --> (n)ms 만큼 대기
	int MAX_EVENTS = 1024;
	struct epoll_event epoll_events[MAX_EVENTS];
	int event_count;
	int timeout = -1;

	int clientCount = 0;
	while (true) {
		event_count = epoll_wait(epoll_fd, epoll_events, MAX_EVENTS, timeout);
		printf("event count[%d]\n", event_count);

		if (event_count < 0) {
			if (errno == EINTR) { // 신호처리기(gdb)에 의해 중단됨
				continue;
			}

			printf("epoll_wait() error [%d]\n", event_count);
			return 0;
		}

		for (int i = 0; i < event_count; i++) {
			// Accept
			if (epoll_events[i].data.fd == server_fd) {
				int client_fd;
				int client_len;
				struct sockaddr_in client_addr;

				printf("User Accept\n");
				client_len = sizeof(client_addr);
				client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);

				// client fd Non-Blocking Socket으로 설정. Edge Trigger 사용하기 위해 설정.
				int flags = fcntl(client_fd, F_GETFL);
				flags |= O_NONBLOCK;
				if (fcntl(client_fd, F_SETFL, flags) < 0) {
					printf("client_fd[%d] fcntl() error\n", client_fd);
					return 0;
				}

				if (client_fd < 0) {
					printf("accept() error [%d]\n", client_fd);
					continue;
				}

				// 클라이언트 fd, epoll 에 등록
				struct epoll_event events;
#ifdef EDGE_TRIGGER 
				// 에지 트리거(Edge Trigger, ET)
				events.events = EPOLLIN | EPOLLET;
#else
				// 레벨 트리거(Level Trigger, LT)
				events.events = EPOLLIN;
#endif
				//events.data.fd = client_fd;
				//
				events.data.ptr = new log4cxx::ext::socket::Client(client_fd);

				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &events) < 0) {
					printf("client epoll_ctl() error\n");
					close(client_fd);
					continue;
				}

				printf("Client Accept client_fd[%d], clientCount[%d]\n", client_fd, ++clientCount);
			} else {
				// epoll에 등록 된 클라이언트들의 send data 처리
				//int client_fd = epoll_events[i].data.fd;
				//
				log4cxx::ext::socket::Client* pClient = (log4cxx::ext::socket::Client*)epoll_events[i].data.ptr;
				int client_fd = *pClient;

#ifdef EDGE_TRIGGER 
				// 엣지 트리거 모드
	#if 1
				int readBytes = 0;
				int result = ioctl(client_fd, FIONREAD, &readBytes);
				if (result < 0) { // 에러
					printf("client_fd[%d] : readBytes = %d, result = %d, ioctl()함수가 실패하여 소켓을 종료한다. (errno = %d)\n", client_fd, readBytes, result, errno);
					close(client_fd);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
				} else if (result == 0) { // 성공
					readBytes = std::max<int>(readBytes, 1);
					std::vector<char> readBuf(readBytes, 0);
					int resultBytes = read(client_fd, &readBuf[0], readBuf.size());
					if (resultBytes < 0) { // 에러
						switch (errno)
						{
						case EWOULDBLOCK: // read 버퍼가 비어있음
							printf("client_fd[%d] : resultBytes = %d, read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)\n", client_fd, resultBytes);
							break;
						default:
							printf("client_fd[%d] : resultBytes = %d, read()함수가 실패하여 소켓을 종료한다. (error = %d)\n", client_fd, resultBytes, errno);
							close(client_fd);
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						}
						break;
					} else if (resultBytes == 0) { // 클라이언트 접속 끊김
						printf("client_fd[%d] : resultBytes = %d, 클라이언트의 접속이 끊겨 소켓을 종료한다.\n", client_fd, resultBytes);
						close(client_fd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						break;
					} else { // 클라이언트 데이터 수신됨
						assert(resultBytes == readBytes && "소켓버퍼의 사이즈 만큼 읽어와야 한다.");
						printf("client_fd[%d] : resultBytes = %d, 클라이언트 데이터 수신됨\n", client_fd, resultBytes);
					}
				} else {
					assert(!"이곳은 들어올수 없다.");
					printf("client_fd[%d] : readByte = %d, result = %d, ioctl()함수의 반환값이 0보다 커서 소켓을 종료한다.\n", client_fd, readBytes, result);
					close(client_fd);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
				}		
	#else
				const size_t BUFFER_LEN = 4096;
				std::vector<char> readBuf(BUFFER_LEN, 0);
				while (true) {
					int resultBytes = read(client_fd, &readBuf[0], readBuf.size());
					if (resultBytes < 0) { // 에러
						switch (errno)
						{
						case EWOULDBLOCK: // read 버퍼가 비어있음
							printf("client_fd[%d] : resultBytes = %d, read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)\n", client_fd, resultBytes);
							break;
						default:
							printf("client_fd[%d] : resultBytes = %d, read()함수가 실패하여 소켓을 종료한다. (error = %d)\n", client_fd, resultBytes, errno);
							close(client_fd);
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						}
						break;
					} else if (resultBytes == 0) { // 클라이언트 접속 끊김
						printf("client_fd[%d] : resultBytes = %d, 클라이언트의 접속이 끊겨 소켓을 종료한다.\n", client_fd, resultBytes);
						close(client_fd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						break;
					} else { // 클라이언트 데이터 수신됨
						printf("client_fd[%d] : resultBytes = %d, 클라이언트 데이터 수신됨\n", client_fd, resultBytes);
					}
				} 
	#endif
#else
				// 레벨 트리거 코드
				const size_t readBytes = 4096;
				std::vector<char> readBuf(readBytes, 0);
				int resultBytes = read(client_fd, &readBuf[0], readBuf.size());
				if (resultBytes < 0) { // 에러
					switch (errno)
					{
					case EWOULDBLOCK: // read 버퍼가 비어있음
						printf("client_fd[%d] : resultBytes = %d, read()함수의 버퍼는 비어있다. (errno = EWOULDBLOCK)\n", client_fd, resultBytes);
						break;
					default:
						printf("client_fd[%d] : resultBytes = %d, read()함수가 실패하여 소켓을 종료한다. (error = %d)\n", client_fd, resultBytes, errno);
						close(client_fd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						//
						delete pClient;
					}
					break;
				} else if (resultBytes == 0) { // 클라이언트 접속 끊김
					printf("client_fd[%d] : resultBytes = %d, 클라이언트의 접속이 끊겨 소켓을 종료한다.\n", client_fd, resultBytes);
					close(client_fd);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
					//
					delete pClient;
					break;
				} else { // 클라이언트 데이터 수신됨
					pClient->forceLog(&readBuf[0], resultBytes);
					printf("client_fd[%d] : resultBytes = %d, 클라이언트 데이터 수신됨\n", client_fd, resultBytes);
				}
#endif
			}
		}
	}
}
