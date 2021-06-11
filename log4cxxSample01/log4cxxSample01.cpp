// log4cxxSample01.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "log4cxxObj.h"
#include <iostream> // std::cerr, std::cout
#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/Exception.h> // log4cxx::helpers::Exception, log4cxx::helpers::RuntimeException

static bool loadFiles()
{
	auto forceLog = [](const std::vector<char>& byteBuf) -> bool {
		try {
			log4cxx::spi::LoggingEventPtr event = log4cxx::factory::createLoggingEvent(byteBuf);
			log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
			if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
				log4cxx::helpers::Pool p;
				remoteLogger->callAppenders(event, p);
			}
		} catch (log4cxx::helpers::RuntimeException& e) {
			std::cout << e.what() << std::endl;
			return false;
		} catch (log4cxx::helpers::Exception& e) {
			std::cout << e.what() << std::endl;
		}

		return true;
	};

	{
		std::vector<char> byteBuf = log4cxx::io::loadFile("LoggingEvent_#1.bin");

		size_t size = 0;
		try {
			size = log4cxx::io::readStart(byteBuf);
		} catch (log4cxx::helpers::RuntimeException& e) {
			std::cerr << e.what() << std::endl;
			return false;
		} catch (log4cxx::helpers::Exception& e) {
			std::cout << e.what() << std::endl;
		}

		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + size);

		std::vector<char> copyBuf;
		size_t byteBufSize = byteBuf.size();
		size_t count = byteBufSize / 100;
		size_t remain = byteBufSize % 100;

		for (int i = 0; i < count; i++) {
			for (int j = 0; j < 100; j++) {
				copyBuf.push_back(byteBuf[j + 100 * i]);
			}
			//
			if (!forceLog(copyBuf)) {
				return false;
			}
		}
		for (int i = 0; i < remain; i++) {
			copyBuf.push_back(byteBuf[i + 100 * count]);
		}
		//
		if (!forceLog(copyBuf)) {
			return false;
		}
	}

	{
		std::vector<char> byteBuf = log4cxx::io::loadFile("LoggingEvent_#2.bin");
		if (!forceLog(byteBuf)) {
			return false;
		}
	}

	{
		std::vector<char> byteBuf = log4cxx::io::loadFile("LoggingEvent_#3.bin");
		if (!forceLog(byteBuf)) {
			return false;
		}
	}

	return true;
}

int main()
{
	setlocale(LC_ALL, "");

	std::string filePath = "log4cxx.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));
	
	loadFiles();

	return 0;
}
