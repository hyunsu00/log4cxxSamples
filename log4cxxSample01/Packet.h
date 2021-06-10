#pragma once
#include <vector> // std::vector
#include <log4cxx/log4cxx.h> // log4cxx_time_t
#include <log4cxx/logstring.h> // LogString
#include <log4cxx/mdc.h> // MDC::Map
#include <log4cxx/spi/loggingevent.h> // LoggingEvent

namespace log4cxx { namespace helpers {

    enum { STREAM_MAGIC = 0xACED };
    enum { STREAM_VERSION = 5 };
    enum {
        TC_NULL = 0x70,
        TC_REFERENCE = 0x71,
        TC_CLASSDESC = 0x72,
        TC_OBJECT = 0x73,
        TC_STRING = 0x74,
        TC_ARRAY = 0x75,
        TC_CLASS = 0x76,
        TC_BLOCKDATA = 0x77,
        TC_ENDBLOCKDATA = 0x78
    };
    enum {
        SC_WRITE_METHOD = 0x01,
        SC_SERIALIZABLE = 0x02
    };

	std::vector<char> loadPacket(const char* filename);

    bool beginPacket(const std::vector<char>& packet);
	bool parsePacket(const std::vector<char>& packet);

    size_t readLong(const std::vector<char>& packet, size_t pos, log4cxx_time_t& value);
    size_t readInt(const std::vector<char>& packet, size_t pos, int& value);
    size_t readLogString(const std::vector<char>& packet, size_t pos, LogString& value);
    size_t readUTFString(const std::vector<char>& packet, size_t pos, std::string& value);
    size_t readObject(const std::vector<char>& packet, size_t pos, MDC::Map& value);


    log4cxx::spi::LoggingEventPtr createLoggingEvent(const std::vector<char>& packet);

}} // namespace log4cxx::helpers
