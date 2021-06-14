// ByteBufInputStream.h
#pragma once
#include <vector> // std::vector
#ifdef _WIN32
class LOG4CXX_EXPORT std::exception; // C4275
#endif
#include <log4cxx/helpers/exception.h> // log4cxx::helpers::RuntimeException
#include <log4cxx/mdc.h> // log4cxx::MDC::Map

namespace log4cxx { namespace ext {

    class SmallBufferException : public log4cxx::helpers::RuntimeException
    {
    public:
        SmallBufferException(const LogString& msg);
        SmallBufferException(const SmallBufferException& msg);
        SmallBufferException& operator=(const SmallBufferException& src);
    }; // class SmallBufferException

    class InvalidBufferException : public log4cxx::helpers::RuntimeException
    {
    public:
        InvalidBufferException(const LogString& msg);
        InvalidBufferException(const InvalidBufferException& msg);
        InvalidBufferException& operator=(const InvalidBufferException& src);
    }; // class InvalidBufferException

}} // log4cxx::ext

namespace log4cxx { namespace ext { namespace io {

    using ByteBuf = std::vector<char>;

    size_t readByte(const ByteBuf& byteBuf, size_t pos, unsigned char& value) /*throw(SmallBufferException)*/;
    size_t readBytes(const ByteBuf& byteBuf, size_t pos, size_t bytes, std::vector<unsigned char>& value) /*throw(SmallBufferException)*/;
    size_t readInt(const ByteBuf& byteBuf, size_t pos, int& value) /*throw(SmallBufferException)*/;
    size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_int64_t& value) /*throw(SmallBufferException)*/;

    size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value, bool skipTypeClass = false) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value, bool skipTypeClass = false) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readObject(const ByteBuf& byteBuf, size_t pos, MDC::Map& value, bool skipTypeClass = false) /*throw(SmallBufferException, InvalidBufferException)*/;

    size_t readProlog(
        const ByteBuf& byteBuf,
        size_t pos, 
        const unsigned char* classDesc,
        size_t classDescLen, 
        std::pair<std::string, unsigned int>& value
    ) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readLocationInfo(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readMDC(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readNDC(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(SmallBufferException, InvalidBufferException)*/;

}}} // log4cxx::ext::io
