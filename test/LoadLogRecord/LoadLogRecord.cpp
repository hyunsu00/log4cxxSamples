// LoadLogRecord.cpp
//
#include <iostream>
#include <msgpack.hpp>
#include <memory> // std::unique_ptr
#include <log4cxx/basicconfigurator.h> // log4cxx::BasicConfigurator
#include <log4cxx/consoleappender.h> // log4cxx::ConsoleAppender
#include <log4cxx/patternlayout.h> // log4cxx::PatternLayout
#include <log4cxx/logmanager.h> // log4cxx::LogManager
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr
#include "BytesObjectLoader.h"
#include "MsgpackObjectLoader.h"

std::vector<char> loadFile(const char* filename)
{
	struct FileCloseDeleter
	{
		inline void operator()(FILE* fp) const
		{
			fclose(fp);
		}
	}; // struct FileCloseDeleter 
	using AutoFilePtr = std::unique_ptr<std::remove_pointer<FILE*>::type, FileCloseDeleter>;

	auto getFileContents = [](const char* filename) -> std::vector<char> {
		FILE* file = fopen(filename, "rb");
		if (!file) {
			fprintf(stderr, "Failed to open: %s\n", filename);
			return std::vector<char>();
		}
		(void)fseek(file, 0, SEEK_END);
		size_t file_length = ftell(file);
		if (!file_length) {
			return std::vector<char>();
		}
		(void)fseek(file, 0, SEEK_SET);

		std::vector<char> buffer(file_length, 0);
		size_t bytes_read = fread(&buffer[0], 1, file_length, file);
		(void)fclose(file);
		if (bytes_read != file_length) {
			fprintf(stderr, "Failed to read: %s\n", filename);
			return std::vector<char>();
		}
		return buffer;
	};

	return getFileContents(filename);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	std::string exeDir;
	std::string msgpackDir;
	std::string binDir;
#ifdef _WIN32	
	{
		char drive[_MAX_DRIVE] = { 0, }; // 드라이브 명
		char dir[_MAX_DIR] = { 0, }; // 디렉토리 경로
		_splitpath_s(argv[0], drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
		exeDir = std::string(drive) + dir;
		msgpackDir = exeDir + "samples\\msgpack\\";
		binDir = exeDir + "samples\\bin\\";
	}
#else
	{
		char* exePath = strdup(argv[0]);
		exeDir = dirname(exePath);
		free(exePath);
		exeDir += "/";
		msgpackDir = exeDir + "samples/msgpack/";
		binDir = exeDir + "samples/bin/";
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

	std::map<::msgpack::type::object_type, std::string> typeMap = {
		{msgpack::type::NIL, "NIL"},
		{msgpack::type::BOOLEAN, "BOOLEAN"},
		{msgpack::type::POSITIVE_INTEGER, "POSITIVE_INTEGER"},
		{msgpack::type::NEGATIVE_INTEGER, "NEGATIVE_INTEGER"},
		{msgpack::type::FLOAT32, "FLOAT32"},
		{msgpack::type::FLOAT64, "FLOAT64"},
		{msgpack::type::FLOAT, "FLOAT"},
		{msgpack::type::STR, "STR"},
		{msgpack::type::BIN, "BIN"},
		{msgpack::type::ARRAY, "ARRAY"},
		{msgpack::type::MAP, "MAP"},
		{msgpack::type::EXT, "EXT"},
	};

	std::vector<char> byteBuf = loadFile((msgpackDir + "record0.msgpak").c_str());

#if 0
	msgpack::unpacker unpacker;
	unpacker.reserve_buffer(byteBuf.size());
	memcpy(unpacker.buffer(), &byteBuf[0], byteBuf.size());
	unpacker.buffer_consumed(byteBuf.size());
	msgpack::object_handle oh;
	while (unpacker.next(oh)) {
		msgpack::object msg = oh.get();
		auto& life = oh.zone();

		auto type = msg.type;
		auto const& msgmap = msg.via.map;
		auto dist = std::distance(begin(msgmap), end(msgmap));
		std::cout << "{" << std::endl;
		for (auto const& kv : msgmap) {
			std::cout << "[" << typeMap[kv.key.type] << "]" << ":" << "[" << typeMap[kv.val.type] << "]" << " => ";
			std::cout << kv.key << ":" << kv.val << "," << std::endl;
		}
		std::cout << "}" << std::endl;

		std::cout << "message reached: " << msg << std::endl;
	}
#endif

#if 1
	byteBuf = loadFile((msgpackDir + "record0.msgpak").c_str());
	log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::msgpack::createLoggingEvent(byteBuf);
	log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
	if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
		log4cxx::helpers::Pool p;
		remoteLogger->callAppenders(event, p);
	}
#endif

#if 1
	byteBuf = loadFile((binDir + "record0.bin").c_str());
	event = log4cxx::ext::loader::bytes::createLoggingEvent(byteBuf);
	remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
	if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
		log4cxx::helpers::Pool p;
		remoteLogger->callAppenders(event, p);
	}
#endif
}
