// example01.cpp
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

int main(int argc, char* argv[])
{
#ifdef _WIN32
	_setmode(_fileno(stdout), _O_U16TEXT);
#endif

	setlocale(LC_ALL, "");

	std::string exeDir;
	{
		char drive[_MAX_DRIVE] = { 0, }; // 드라이브 명
		char dir[_MAX_DIR] = { 0, }; // 디렉토리 경로
		_splitpath_s(argv[0], drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
		exeDir = std::string(drive) + dir;
	}
	std::string filePath = exeDir + "shortSocketClient.conf";

	try {
		while (true) {
			log4cxx::PropertyConfigurator::configure(log4cxx::File(filePath));

			log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
			{
				// 빌드시 윈도우는 디폴트 WCHAR, 리눅스는 UTF-8로 처리하므로 
				// 리눅스에서 LogString은 CHAR, 윈도우에서는 LogString은 WCHAR이다.
				// 따라서 리눅스에서 WCHAR 출력을 위해서는 L을 꼭 써야 한다.
				LOG4CXX_TRACE(rootLogger, L"[" << L"rootLogger(wchar_t)" << L"] : " << L"TRACE 출력 שלום (שלום)...");
				LOG4CXX_DEBUG(rootLogger, L"[" << L"rootLogger(wchar_t)" << L"] : " << L"DEBUG 출력 Сәлем (Sälem)...");
				LOG4CXX_INFO(rootLogger, L"[" << L"rootLogger(wchar_t)" << L"] : " << L"INFO 출력 مرحبًا (mrhbana)...");
				LOG4CXX_WARN(rootLogger, L"[" << L"rootLogger(wchar_t)" << L"] : " << L"WARN 출력 안녕하세요 (annyeonghaseyo)...");
				LOG4CXX_ERROR(rootLogger, L"[" << L"rootLogger(wchar_t)" << L"] : " << L"ERROR 출력 こんにちは (Kon'nichiwa)...");
				LOG4CXX_FATAL(rootLogger, L"[" << L"rootLogger(wchar_t)" << L"] : " << L"FATAL 출력 你好 (Nǐ hǎo)...");

				LOG4CXX_TRACE(rootLogger, LOG4CXX_STR("[") << rootLogger->getName() << LOG4CXX_STR("(LogString)") << LOG4CXX_STR("] : ") << LOG4CXX_STR("TRACE 출력 שלום (שלום)..."));
				LOG4CXX_DEBUG(rootLogger, LOG4CXX_STR("[") << rootLogger->getName() << LOG4CXX_STR("(LogString)") << LOG4CXX_STR("] : ") << LOG4CXX_STR("DEBUG 출력 Сәлем (Sälem)..."));
				LOG4CXX_INFO(rootLogger, LOG4CXX_STR("[") << rootLogger->getName() << LOG4CXX_STR("(LogString)") << LOG4CXX_STR("] : ") << LOG4CXX_STR("INFO 출력 مرحبًا (mrhbana)..."));
				LOG4CXX_WARN(rootLogger, LOG4CXX_STR("[") << rootLogger->getName() << LOG4CXX_STR("(LogString)") << LOG4CXX_STR("] : ") << LOG4CXX_STR("WARN 출력 안녕하세요 (annyeonghaseyo)..."));
				LOG4CXX_ERROR(rootLogger, LOG4CXX_STR("[") << rootLogger->getName() << LOG4CXX_STR("(LogString)") << LOG4CXX_STR("] : ") << LOG4CXX_STR("ERROR 출력 こんにちは (Kon'nichiwa)..."));
				LOG4CXX_FATAL(rootLogger, LOG4CXX_STR("[") << rootLogger->getName() << LOG4CXX_STR("(LogString)") << LOG4CXX_STR("] : ") << LOG4CXX_STR("FATAL 출력 你好 (Nǐ hǎo)..."));
			}
			std::this_thread::sleep_for(std::chrono::duration<int>(1));
		}
	} catch (const log4cxx::helpers::Exception& e) {
		std::cout << "[ERROR] configure()\n" << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
