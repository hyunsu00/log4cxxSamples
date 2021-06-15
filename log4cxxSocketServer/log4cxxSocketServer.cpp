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

const char *const SERVER_LOGGER = "serverLogger";
inline log4cxx::LoggerPtr serverLogger()
{
	return log4cxx::Logger::getLogger(SERVER_LOGGER);
}
using LogLog = log4cxx::helpers::LogLog;

auto loadFiles = [](const std::string &sampleDir) -> bool
{
	auto forceLog = [](const std::vector<char> &byteBuf) -> bool
	{
		try {
			std::vector<char> copyByteBuf = byteBuf;
			log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::createLoggingEvent(copyByteBuf);
			log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
			if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
				log4cxx::helpers::Pool p;
				remoteLogger->callAppenders(event, p);
			}
		} catch (log4cxx::ext::SmallBufferException &e) {
			LOG4CXX_WARN(serverLogger(), e.what());
		} catch (log4cxx::ext::InvalidBufferException &e) {
			LOG4CXX_ERROR(serverLogger(), e.what());
			return false;
		}

		return true;
	}; // auto forceLog

	{
		std::vector<char> byteBuf = log4cxx::ext::io::loadFile((sampleDir + "LoggingEvent_#1.bin").c_str());

		size_t size = 0;
		try {
			size = log4cxx::ext::loader::readStart(byteBuf);
		} catch (log4cxx::ext::SmallBufferException &e) {
			LOG4CXX_WARN(serverLogger(), e.what());
		} catch (log4cxx::ext::InvalidBufferException &e) {
			LOG4CXX_ERROR(serverLogger(), e.what());
			return false;
		}

		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + size);

		std::vector<char> copyBuf;
		size_t byteBufSize = byteBuf.size();
		size_t count = byteBufSize / 100;
		size_t remain = byteBufSize % 100;

		for (size_t i = 0; i < count; i++) {
			for (size_t j = 0; j < 100; j++) {
				copyBuf.push_back(byteBuf[j + 100 * i]);
			}
			// 로그 출력
			if (!forceLog(copyBuf)) {
				return false;
			}
		}
		for (size_t i = 0; i < remain; i++) {
			copyBuf.push_back(byteBuf[i + 100 * count]);
		}
		// 로그 출력
		if (!forceLog(copyBuf))
		{
			return false;
		}
	}

	{
		std::vector<char> byteBuf = log4cxx::ext::io::loadFile((sampleDir + "LoggingEvent_#2.bin").c_str());
		if (!forceLog(byteBuf)) {
			return false;
		}
	}

	{
		std::vector<char> byteBuf = log4cxx::ext::io::loadFile((sampleDir + "LoggingEvent_#3.bin").c_str());
		if (!forceLog(byteBuf)) {
			return false;
		}
	}

	return true;
}; // auto loadFiles

auto runClient = [](SOCKET clientSocket, const std::string &clientInfo)
{
	LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 접속 - ") << clientInfo.c_str());

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

		LOG4CXX_ASSERT(serverLogger(), len == readBytes, LOG4CXX_STR("자바 스트림 프로토콜 크기는 4byte 이여야 한다."));
		if (len != readBytes) {
			goto CLEAN_UP;
		}

		try {
			log4cxx::ext::loader::readStart(recvBuf);
		} catch (log4cxx::helpers::Exception &e) {
			LOG4CXX_ERROR(serverLogger(), e.what());
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
					LOG4CXX_WARN(serverLogger(), e.what());
					break;
				} catch (log4cxx::ext::InvalidBufferException& e) { // 종료
					LOG4CXX_ERROR(serverLogger(), e.what());
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
				LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] : [recvBytes = ") << recvBytes << LOG4CXX_STR(" ] 종료중... "));
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
				LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 [") << clientInfo.c_str() << LOG4CXX_STR("] : 종료중... "));
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
	log4cxx::ext::socket::Close(clientSocket);
	LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 종료 - ") << clientInfo.c_str());
}; // auto runClient

auto runServer = [](int port_num) -> void
{
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
	if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("바인딩 실패."));
		return;
	}

	// 소켓 리슨
	if (listen(serverSocket, 5) < 0) {
		LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("리슨 실패."));
		return;
	}
	LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("포트 = ") << port_num);
	LOG4CXX_INFO(serverLogger(), LOG4CXX_STR("클라이언트 접속 요청 대기중..."));

	while (true) {
		struct sockaddr_in clientAddr;
		int len = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, (socklen_t *)&len);
		if (clientSocket < 0) {
			LOG4CXX_FATAL(serverLogger(), LOG4CXX_STR("accept 시도 실패."));
			break;
		}

		std::string clientInfo = log4cxx::ext::socket::getClientInfo(clientAddr);

		std::thread clientThread(runClient, clientSocket, clientInfo);
		clientThread.detach();
	}

	log4cxx::ext::socket::Close(serverSocket);
}; // auto runServer

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	std::string exeDir;
#ifdef _WIN32	
	{
		char drive[_MAX_DRIVE] = {
			0,
		}; // 드라이브 명
		char dir[_MAX_DIR] = {
			0,
		}; // 디렉토리 경로
		_splitpath_s(argv[0], drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
		exeDir = std::string(drive) + dir;
	}
#else
	{
		char *exePath = strdup(argv[0]);
		exeDir = dirname(exePath);
		free(exePath);
		exeDir += "/";
	}
#endif

	std::string filePath = exeDir + "log4cxxSocketServer.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

	// loadFiles(exeDir + "samples\\");

	log4cxx::ext::socket::Init();

	{
		runServer(4445);
	}

	log4cxx::ext::socket::Quit();

	return 0;
}
