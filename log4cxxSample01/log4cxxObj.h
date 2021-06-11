// log4cxxObj.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/log4cxx.h> // log4cxx_time_t
#include <log4cxx/logstring.h> // LogString
#include <log4cxx/mdc.h> // MDC::Map
#include <log4cxx/spi/loggingevent.h> // LoggingEvent
#include <log4cxx/spi/location/locationinfo.h> // LocationInfo
#include <memory> // std::shared_ptr

namespace log4cxx { namespace spi {
    typedef std::shared_ptr<LocationInfo> LocationInfoPtr;
}}

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

    // 자바 스트림 프로토콜 반환
    size_t readStart(const std::vector<char>& packet) /*throw(exception, logic_error)*/;

    log4cxx::spi::LoggingEventPtr createLoggingEvent(const std::vector<char>& packet) /*throw(exception, logic_error, bad_alloc)*/;
    log4cxx::spi::LocationInfoPtr createLocationInfo(const std::string& fullInfo) /*throw(bad_alloc)*/;

    size_t readProlog(
        const std::vector<char>& packet, 
        size_t pos, 
        const unsigned char* classDesc,
        size_t classDescLen, 
        std::pair<std::string, unsigned int>& value
    ) /*throw(exception, logic_error)*/;
    size_t readLocationInfo(const std::vector<char>& packet, size_t pos, std::string& value) /*throw(exception, logic_error)*/;
    size_t readMDC(const std::vector<char>& packet, size_t pos, MDC::Map& value) /*throw(exception, logic_error)*/;
    size_t readNDC(const std::vector<char>& packet, size_t pos, LogString& value) /*throw(exception, logic_error)*/;
    size_t readObject(const std::vector<char>& packet, size_t pos, MDC::Map& value) /*throw(exception, logic_error)*/;

    size_t readByte(const std::vector<char>& packet, size_t pos, unsigned char& value) /*throw(exception)*/;
    size_t readBytes(const std::vector<char>& packet, size_t pos, size_t bytes, std::vector<char>& value) /*throw(exception)*/;
    size_t readLong(const std::vector<char>& packet, size_t pos, log4cxx_time_t& value) /*throw(exception)*/;
    size_t readInt(const std::vector<char>& packet, size_t pos, int& value) /*throw(exception)*/;
    size_t readLogString(const std::vector<char>& packet, size_t pos, LogString& value) /*throw(exception, logic_error)*/;
    size_t readUTFString(const std::vector<char>& packet, size_t pos, std::string& value) /*throw(exception, logic_error)*/;
    
}} // namespace log4cxx::helpers
