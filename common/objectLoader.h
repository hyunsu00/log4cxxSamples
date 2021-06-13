// ObjectLoader.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/helpers/exception.h> // log4cxx::helpers::RuntimeException
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr

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

    ByteBuf loadFile(const char* filename);

    // 자바 스트림 프로토콜 반환
    size_t readStart(const ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;

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

    size_t readObject(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readByte(const ByteBuf& byteBuf, size_t pos, unsigned char& value) /*throw(SmallBufferException)*/;
    size_t readBytes(const ByteBuf& byteBuf, size_t pos, size_t bytes, std::vector<char>& value) /*throw(SmallBufferException)*/;
    size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_time_t& value) /*throw(SmallBufferException)*/;
    size_t readInt(const ByteBuf& byteBuf, size_t pos, int& value) /*throw(SmallBufferException)*/;
    size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(SmallBufferException, InvalidBufferException)*/;
    size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(SmallBufferException, InvalidBufferException)*/;
    
}}} // log4cxx::ext::io

namespace log4cxx { namespace ext { namespace loader {

    using ByteBuf = io::ByteBuf;

    log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;

}}} // log4cxx::ext::loader
