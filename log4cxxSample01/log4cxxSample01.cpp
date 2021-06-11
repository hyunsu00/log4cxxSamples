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

#include "Packet.h"
#include <crtdbg.h> // _ASSERTE

int main()
{
#ifdef _WIN32
	_setmode(_fileno(stdout), _O_U16TEXT);
#endif

	setlocale(LC_ALL, "");

	std::string filePath = "log4cxx.conf";
	log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

	log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();

	{
		std::vector<char> packet = log4cxx::helpers::loadPacket("packet_#1.bin");

		bool ret = log4cxx::helpers::beginPacket(packet);
		_ASSERTE(ret && "beginPacket() failed");

		packet.erase(packet.begin(), packet.begin() + 4);
		ret = log4cxx::helpers::parsePacket(packet);
		_ASSERTE(ret && "parsePacket() failed");
	}
	
	{
		std::vector<char> packet = log4cxx::helpers::loadPacket("packet_#2.bin");

		bool ret = log4cxx::helpers::parsePacket(packet);

		log4cxx::helpers::createLoggingEvent(packet);
		_ASSERTE(ret && "parsePacket() failed");
	}

	{
		std::vector<char> packet = log4cxx::helpers::loadPacket("packet_#2.bin");

		bool ret = log4cxx::helpers::parsePacket(packet);

		log4cxx::helpers::createLoggingEvent(packet);
		_ASSERTE(ret && "parsePacket() failed");
	}
}
