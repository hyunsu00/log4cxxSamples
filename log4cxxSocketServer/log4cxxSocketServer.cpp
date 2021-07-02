// log4cxxSocketServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/loglog.h>		  // log4cxx::helpers::LogLog
#include <thread>						  // std::thread

#include "log4cxxSocket.h"

#ifdef _WIN32
#else
#	include <string.h>	// strdup
#	include <libgen.h>	// dirname
#endif

#include "ByteBufInputStream.h"
#include "ObjectLoader.h"

const char* const SERVER_LOGGER = "serverLogger";
static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getLogger(SERVER_LOGGER);
using LogLog = log4cxx::helpers::LogLog;

auto runClient = [](SOCKET clientSocket, const std::string &clientInfo)
{
	LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 접속 - ") << clientInfo.c_str());

#if 0

	const size_t BUF_LEN = 4096;
	std::vector<char> recvBuf(BUF_LEN, 0);
	// 자바 스트림 프로토콜 확인
	{
		// 4byte를 강제로 읽는다.
		char* pBuf = &recvBuf[0];
		const size_t len = 4;
		size_t readBytes = 0;
		while (readBytes < len) {
			int len_read = ::recv(clientSocket, pBuf, len - readBytes, 0);
			if (len_read <= 0) {
				goto CLEAN_UP;
			}

			pBuf += len_read;
			readBytes = static_cast<size_t>(pBuf - &recvBuf[0]);
		}

		LOG4CXX_ASSERT(sLogger, len == readBytes, LOG4CXX_STR("자바 스트림 프로토콜 크기는 4byte 이여야 한다."));
		if (len != readBytes) {
			goto CLEAN_UP;
		}

		try {
			log4cxx::ext::loader::readStart(recvBuf);
		} catch (log4cxx::helpers::Exception &e) {
			LOG4CXX_ERROR(sLogger, e.what());
			goto CLEAN_UP;
		}
	}

	// LoggingEvent
	{
		auto forceLog = [](std::vector<char>& byteBuf) -> bool {
			while (!byteBuf.empty()) {
				try {
					log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::createLoggingEvent(byteBuf);
					log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
					if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
						log4cxx::helpers::Pool p;
						remoteLogger->callAppenders(event, p);
					}
				} catch (log4cxx::ext::SmallBufferException& e) { // 무시
					LOG4CXX_WARN(sLogger, e.what());
					break;
				} catch (log4cxx::ext::InvalidBufferException& e) { // 종료
					LOG4CXX_ERROR(sLogger, e.what());
					return false;
				}
			}

			return true;
		};

		std::vector<char> byteBuf;
		while (true) {
			int recvBytes = recv(clientSocket, &recvBuf[0], static_cast<int>(recvBuf.size()), 0);
			// recvBytes == 0 일 경우 -> 정상적인 종료
			// recvBytes == SOCKET_ERROR(-1) 일 경우 -> 소켓에러 WSAGetLastError() 호출하여 오류코드 검색 가능
			if (recvBytes <= 0) {
				LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] : [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] 종료중... "));
				goto CLEAN_UP;
			}

			byteBuf.insert(byteBuf.end(), recvBuf.begin(), recvBuf.begin() + recvBytes);
			if (!forceLog(byteBuf)) {
				goto CLEAN_UP;
			}
		}
	}

#else

	// 자바 스트림 프로토콜 확인
	{
		bool start = log4cxx::ext::loader::readStart(clientSocket);
		if (!start) {
			goto CLEAN_UP;
		}
	}

	// LoggingEvent
	{
		while (true) {
			log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::createLoggingEvent(clientSocket);
			if (!event) {
				LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] : 종료중... "));
				goto CLEAN_UP;
			}
			log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
			if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
				log4cxx::helpers::Pool p;
				remoteLogger->callAppenders(event, p);
			}
		}
	}

#endif

CLEAN_UP:
	closesocket(clientSocket);
	LOG4CXX_DEBUG(sLogger, LOG4CXX_STR("클라이언트 종료 - ") << clientInfo.c_str());
}; // auto runClient

// https://sourceware.org/bugzilla/show_bug.cgi?id=17082
// auto runServer = [](int port) -> void
static void runServer(int port)
{
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓을 못열었다."));
		return;
	}

	// 소켓 옵션 설정.
	// Option -> SO_REUSEADDR : 비정상 종료시 해당 포트 재사용 가능하도록 설정
	int option = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
		closesocket(serverSocket);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("소켓옵션(SO_REUSEADDR) 실패."));
		return;
	}

	// Nagle 알고리즘 끄기
	if (setsockopt(serverSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option)) < 0) {
		closesocket(serverSocket);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("Nagle 알고리즘 OFF(TCP_NODELAY) 실패."));
		return;
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0x00, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 소켓 주소 자동 할당
	serverAddr.sin_port = htons(port);	// 서버 포트 설정

	// 소켓 바인딩
	if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		closesocket(serverSocket);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("바인딩 실패."));
		return;
	}

	// 소켓 리슨
	if (listen(serverSocket, 5) < 0) {
		closesocket(serverSocket);
		LOG4CXX_FATAL(sLogger, LOG4CXX_STR("리슨 실패."));
		return;
	}
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("포트 = ") << port);
	LOG4CXX_INFO(sLogger, LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

	while (true) {
		struct sockaddr_in clientAddr;
		int len = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, (socklen_t *)&len);
		if (clientSocket < 0) {
			LOG4CXX_FATAL(sLogger, LOG4CXX_STR("accept 시도 실패."));
			break;
		}

		std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientAddr);

		std::thread clientThread(runClient, clientSocket, clientInfo);
		clientThread.detach();
	}

	closesocket(serverSocket);
}; // auto runServer

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	std::string exeDir;
	std::string sampleDir;
#ifdef _WIN32	
	{
		char drive[_MAX_DRIVE] = {0, }; // 드라이브 명
		char dir[_MAX_DIR] = {0, }; // 디렉토리 경로
		_splitpath_s(argv[0], drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
		exeDir = std::string(drive) + dir;
		sampleDir = exeDir + "samples\\";
	}
#else
	{
		char *exePath = strdup(argv[0]);
		exeDir = dirname(exePath);
		free(exePath);
		exeDir += "/";
		sampleDir = exeDir + "samples/";
	}
#endif

	std::string filePath = exeDir + "log4cxxSocketServer.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

	log4cxx::ext::socket::Init();

	{
		runServer(9988);
	}

	log4cxx::ext::socket::Quit();

	return 0;
}
