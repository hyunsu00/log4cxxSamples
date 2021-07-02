// LoadLoggingEvent.cpp
//
#include <log4cxx/basicconfigurator.h> // log4cxx::BasicConfigurator
#include <log4cxx/consoleappender.h> // log4cxx::ConsoleAppender
#include <log4cxx/patternlayout.h> // log4cxx::PatternLayout
#include <log4cxx/logmanager.h> // log4cxx::LogManager

#ifdef _WIN32
#	include <winsock2.h> // SOCKET
#	include <crtdbg.h> // _ASSERTE
#else
#	include <assert.h> // assert
#	define _ASSERTE assert
#	include <string.h>	// strdup
#	include <libgen.h>	// dirname
#endif

#include "ByteBufInputStream.h"
#include "ObjectLoader.h"

static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getRootLogger();

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
			LOG4CXX_WARN(sLogger, e.what());
		} catch (log4cxx::ext::InvalidBufferException &e) {
			LOG4CXX_ERROR(sLogger, e.what());
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
			LOG4CXX_WARN(sLogger, e.what());
		} catch (log4cxx::ext::InvalidBufferException &e) {
			LOG4CXX_ERROR(sLogger, e.what());
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
		sampleDir = exeDir + "samples\\javaobj\\";
	}
#else
	{
		char *exePath = strdup(argv[0]);
		exeDir = dirname(exePath);
		free(exePath);
		exeDir += "/";
		sampleDir = exeDir + "samples/javaobj/";
	}
#endif

	// log4cxx::BasicConfigurator::configure();
	log4cxx::ConsoleAppenderPtr appender(new log4cxx::ConsoleAppender());
	log4cxx::LayoutPtr layout(new log4cxx::PatternLayout(LOG4CXX_STR("%5p %F\:%L [%d] - %m%n")));
	appender->setLayout(layout);
	log4cxx::helpers::Pool pool;
	appender->activateOptions(pool);
	log4cxx::Logger::getRootLogger()->addAppender(appender);
	log4cxx::LogManager::getLoggerRepository()->setConfigured(true);

	loadFiles(sampleDir);

	return 0;
}
