// SaveLoggingEvent.cpp
//
#include <iostream>
#include <fstream>
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

#include "DefaultObjectSaver.h"
#include "DefaultObjectLoader.h"

static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getRootLogger();
auto forceLog = [](const std::vector<char>& byteBuf) -> bool
{
	try {
		std::vector<char> copyByteBuf = byteBuf;
		log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::Default::createLoggingEvent(copyByteBuf);
		log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
		if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
			log4cxx::helpers::Pool p;
			remoteLogger->callAppenders(event, p);
		}
	} catch (log4cxx::ext::SmallBufferException& e) {
		LOG4CXX_WARN(sLogger, e.what());
	} catch (log4cxx::ext::InvalidBufferException& e) {
		LOG4CXX_ERROR(sLogger, e.what());
		return false;
	}

	return true;
}; // auto forceLog

int main()
{
	setlocale(LC_ALL, "");

	// log4cxx::BasicConfigurator::configure();
	log4cxx::ConsoleAppenderPtr appender(new log4cxx::ConsoleAppender());
	log4cxx::LayoutPtr layout(new log4cxx::PatternLayout(LOG4CXX_STR("%5p %F\\:%L [%d] - %m%n")));
	appender->setLayout(layout);
	log4cxx::helpers::Pool pool;
	appender->activateOptions(pool);
	log4cxx::Logger::getRootLogger()->addAppender(appender);
	log4cxx::LogManager::getLoggerRepository()->setConfigured(true);

	using namespace log4cxx::ext::saver::Default;

	{
		log4cxx::ext::LoggingEventData eventData;
		eventData.m_LoggerName = LOG4CXX_STR("root");
		eventData.m_Level = 50000;
		eventData.m_Message = LOG4CXX_STR("123");
		eventData.m_Timestamp = 1623145275089000;
		eventData.m_ThreadName = LOG4CXX_STR("0x00006158");
		eventData.m_PathName = "d:\\github\\hyunsu00\\log4cxxsamples\\examples\\example01.cpp";
		eventData.m_FuncName = "__cdecl main(void)";
		eventData.m_LineNumber = "52";

		std::vector<char> byteBuf = createLoggingEvent(eventData);

		std::ofstream writeFile("123", std::ios::binary);
		writeFile.write(&byteBuf[0], byteBuf.size());
		writeFile.close();

		forceLog(byteBuf);
	}
    
	{
		std::ifstream readFile("123", std::ios::binary | std::ios::ate);
		std::vector<char> byteBuf(readFile.tellg(), 0);
		readFile.seekg(0);
		readFile.read(&byteBuf[0], byteBuf.size());
		readFile.close();

		forceLog(byteBuf);
	}

	return 0;
}
