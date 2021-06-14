// socketInputStream.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/logstring.h> // log4cxx::LogString
#include <log4cxx/mdc.h> // log4cxx::MDC::Map

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

}}} // log4cxx::ext::io
