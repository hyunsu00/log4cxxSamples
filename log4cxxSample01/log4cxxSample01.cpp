// log4cxxSample01.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#ifdef _WIN32
#include <io.h> // _setmode
#include <fcntl.h> // _O_U16TEXT
#endif
#include <locale.h> // setlocale
#include <iostream> // std::cout
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::duration
#include <log4cxx/stream.h> // log4cxx::logstream
#include <log4cxx/basicconfigurator.h> // log4cxx::BasicConfigurator
#include <log4cxx/helpers/exception.h> // log4cxx::helpers::Exception
#include <log4cxx/ndc.h> // log4cxx::NDC
#include <log4cxx/propertyconfigurator.h> // log4cxx::PropertyConfigurator
#include <log4cxx/helpers/exception.h> // log4cxx::helpers::Exception
#include <log4cxx/simplelayout.h> // log4cxx::SimpleLayout
#include <log4cxx/helpers/pool.h> // log4cxx::helpers::Pool

#include "Packet.h"
#include <crtdbg.h> // _ASSERTE

int main()
{
//#ifdef _WIN32
//	_setmode(_fileno(stdout), _O_U16TEXT);
//#endif

	setlocale(LC_ALL, "");

	std::string filePath = "log4cxx.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

	log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();

	{
		std::vector<char> packet = log4cxx::helpers::loadPacket("packet_#1.bin");
		
		size_t size = 0;
		try {
			size = log4cxx::helpers::readStart(packet);
		} catch (std::logic_error& e) {
			std::cerr << e.what() << std::endl;
			return -1;
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		
		packet.erase(packet.begin(), packet.begin() + size);

		std::vector<char> copyPacket;
		size_t packetSize = packet.size();
		int count = packetSize / 100;
		int remain = packetSize % 100;

		for (int i = 0; i < count; i++) {
			for (int j = 0; j < 100; j++) {
				copyPacket.push_back(packet[j + 100 * i]);
			}
			//
			try {
				log4cxx::spi::LoggingEventPtr event = log4cxx::helpers::createLoggingEvent(copyPacket);
				log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
				if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
					log4cxx::helpers::Pool p;
					remoteLogger->callAppenders(event, p);
				}
			} catch (std::logic_error& e) {
				std::cerr << e.what() << std::endl;
				return -1;
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
			}
			
		}
		for (int i = 0; i < remain; i++) {
			copyPacket.push_back(packet[i + 100 * count]);
		}
		//
		try {
			log4cxx::spi::LoggingEventPtr event = log4cxx::helpers::createLoggingEvent(copyPacket);
			log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
			if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
				log4cxx::helpers::Pool p;
				remoteLogger->callAppenders(event, p);
			}
		} catch (std::logic_error& e) {
			std::cerr << e.what() << std::endl;
			return -1;
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}
	
# if 0
	{
		std::vector<char> packet = log4cxx::helpers::loadPacket("packet_#2.bin");

		log4cxx::spi::LoggingEventPtr event = log4cxx::helpers::createLoggingEvent(packet);
		log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
		if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
			log4cxx::helpers::Pool p;
			remoteLogger->callAppenders(event, p);
		}
	}

	{
		std::vector<char> packet = log4cxx::helpers::loadPacket("packet_#2.bin");

		log4cxx::spi::LoggingEventPtr event = log4cxx::helpers::createLoggingEvent(packet);
		log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
		if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
			log4cxx::helpers::Pool p;
			remoteLogger->callAppenders(event, p);
		}
	}
#endif
}
