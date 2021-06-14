// socketLoader.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr

namespace log4cxx { namespace ext { namespace io {

    bool read(int socket, void* buf, size_t len) noexcept;
    bool readByte(int socket, unsigned char& value) noexcept;
    bool readBytes(int socket, size_t bytes, std::vector<unsigned char>& value) noexcept;
    bool readInt(int socket, int& value) noexcept;
    bool readLong(int socket, log4cxx_time_t& value) noexcept;

    bool readUTFString(int socket, std::string& value, bool skipTypeClass = false) noexcept;
    bool readLogString(int socket, LogString& value, bool skipTypeClass = false) noexcept;
    bool readObject(int socket, MDC::Map& value, bool skipTypeClass = false) noexcept;

    bool readProlog(
        int socket,
        const unsigned char* classDesc,
        size_t classDescLen, 
        std::pair<std::string, unsigned int>& value
    ) noexcept;
    bool readLocationInfo(int socket, std::string& value) noexcept;
    bool readMDC(int socket, MDC::Map& value) noexcept;
    bool readNDC(int socket, LogString& value) noexcept;

    // 자바 스트림 프로토콜 반환
    bool readStart(int socket) noexcept;

}}} // log4cxx::ext::io

namespace log4cxx { namespace ext { namespace loader {

    log4cxx::spi::LoggingEventPtr createLoggingEvent(int socket) noexcept;

}}} // log4cxx::ext::loader
