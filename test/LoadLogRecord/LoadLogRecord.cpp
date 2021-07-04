// LoadLogRecord.cpp
//
#include <iostream>

#include <log4cxx/basicconfigurator.h> // log4cxx::BasicConfigurator
#include <log4cxx/consoleappender.h> // log4cxx::ConsoleAppender
#include <log4cxx/patternlayout.h> // log4cxx::PatternLayout
#include <log4cxx/logmanager.h> // log4cxx::LogManager

#include <msgpack.hpp> // msgpack::unpacker

#include "FileLoader.h"
#include "BytesObjectLoader.h"
#include "MsgpackObjectLoader.h"

static log4cxx::LoggerPtr sLogger = log4cxx::Logger::getRootLogger();
auto forceLog = [](const std::vector<char>& byteBuf) -> bool
{
	try {
		std::vector<char> copyByteBuf = byteBuf;
		log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::Msgpack::createLoggingEvent(copyByteBuf);
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
	log4cxx::LayoutPtr layout(new log4cxx::PatternLayout(LOG4CXX_STR("%5p %F\\:%L [%d] - %m%n")));
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

	std::vector<char> byteBuf = log4cxx::ext::io::loadFile((msgpackDir + "record0.msgpak").c_str());

#if 1
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
	byteBuf = log4cxx::ext::io::loadFile((msgpackDir + "record0.msgpak").c_str());

	std::vector<char> copyBuf;
	size_t byteBufSize = byteBuf.size();
	size_t count = byteBufSize / 100;
	size_t remain = byteBufSize % 100;

	for (size_t i = 0; i < count; i++) {
		for (size_t j = 0; j < 100; j++) {
			copyBuf.push_back(byteBuf[j + 100 * i]);
		}
		// 로그 출력
		forceLog(copyBuf);
	}
	for (size_t i = 0; i < remain; i++) {
		copyBuf.push_back(byteBuf[i + 100 * count]);
	}
	// 로그 출력
	forceLog(copyBuf);
#endif

#if 0
	byteBuf = log4cxx::ext::io::loadFile((binDir + "record0.bin").c_str());
	event = log4cxx::ext::loader::Bytes::createLoggingEvent(byteBuf);
	remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
	if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
		log4cxx::helpers::Pool p;
		remoteLogger->callAppenders(event, p);
	}
#endif
}
