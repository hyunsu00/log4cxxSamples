// SocketInputStream.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/logstring.h> // log4cxx::LogString
#include <log4cxx/mdc.h> // log4cxx::MDC::Map

namespace log4cxx { namespace ext { namespace io {

    bool read(SOCKET socket, void* buf, size_t len) noexcept;
    bool readByte(SOCKET socket, unsigned char& value) noexcept;
    bool readBytes(SOCKET socket, size_t bytes, std::vector<unsigned char>& value) noexcept;
    bool readInt(SOCKET socket, int& value) noexcept;
    bool readLong(SOCKET socket, log4cxx_int64_t& value) noexcept;

    bool readUTFString(SOCKET socket, std::string& value, bool skipTypeClass = false) noexcept;
    bool readLogString(SOCKET socket, LogString& value, bool skipTypeClass = false) noexcept;
    bool readObject(SOCKET socket, MDC::Map& value, bool skipTypeClass = false) noexcept;

    bool readProlog(
        SOCKET socket,
        const unsigned char* classDesc,
        size_t classDescLen, 
        std::pair<std::string, unsigned int>& value
    ) noexcept;
    bool readLocationInfo(SOCKET socket, std::string& value) noexcept;
    bool readMDC(SOCKET socket, MDC::Map& value) noexcept;
    bool readNDC(SOCKET socket, LogString& value) noexcept;

}}} // log4cxx::ext::io
