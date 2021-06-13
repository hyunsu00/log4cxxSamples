// socketLoader.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/helpers/exception.h> // log4cxx::helpers::RuntimeException
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr

namespace log4cxx { namespace ext { namespace io {

    using ByteBuf = std::vector<char>;

    bool read(int socket, void* buf, size_t len);
    bool readByte(int socket, unsigned char& value);
    bool readBytes(int socket, size_t bytes, std::vector<unsigned char>& value);
    bool readInt(int socket, int& value);
    bool readLong(int socket, log4cxx_time_t& value);
    bool readUTFString(int socket, std::string& value, bool skipTypeClass);
    bool readLogString(int socket, LogString& value, bool skipTypeClass);
    bool readObject(int socket, MDC::Map& value, bool skipTypeClass);

    // 자바 스트림 프로토콜 반환
    bool readStart(int socket);

    bool readProlog(
        int socket,
        const unsigned char* classDesc,
        size_t classDescLen, 
        std::pair<std::string, unsigned int>& value
    );
    bool readLocationInfo(int socket, std::string& value);
    bool readMDC(int socket, MDC::Map& value);
    bool readNDC(int socket, LogString& value);

}}} // log4cxx::ext::io

namespace log4cxx { namespace ext { namespace loader {

    log4cxx::spi::LoggingEventPtr createLoggingEvent(int socket);

}}} // log4cxx::ext::loader
