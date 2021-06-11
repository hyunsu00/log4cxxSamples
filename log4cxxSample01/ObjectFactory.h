// ObjectFactory.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr

namespace log4cxx { namespace io {

    using ByteBuf = std::vector<char>;

    ByteBuf loadFile(const char* filename);

    // 자바 스트림 프로토콜 반환
    size_t readStart(const ByteBuf& byteBuf) /*throw(InstantiationException, RuntimeException)*/;

    size_t readProlog(
        const ByteBuf& byteBuf,
        size_t pos, 
        const unsigned char* classDesc,
        size_t classDescLen, 
        std::pair<std::string, unsigned int>& value
    ) /*throw(InstantiationException, RuntimeException)*/;
    size_t readLocationInfo(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(InstantiationException, RuntimeException)*/;
    size_t readMDC(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(InstantiationException, RuntimeException)*/;
    size_t readNDC(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(InstantiationException, RuntimeException)*/;

    size_t readObject(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(InstantiationException, RuntimeException)*/;
    size_t readByte(const ByteBuf& byteBuf, size_t pos, unsigned char& value) /*throw(InstantiationException)*/;
    size_t readBytes(const ByteBuf& byteBuf, size_t pos, size_t bytes, std::vector<char>& value) /*throw(InstantiationException)*/;
    size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_time_t& value) /*throw(InstantiationException)*/;
    size_t readInt(const ByteBuf& byteBuf, size_t pos, int& value) /*throw(InstantiationException)*/;
    size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(InstantiationException, RuntimeException)*/;
    size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(InstantiationException, RuntimeException)*/;
    
}} // log4cxx::io

namespace log4cxx { namespace factory {

    using ByteBuf = io::ByteBuf;

    log4cxx::spi::LoggingEventPtr createLoggingEvent(const ByteBuf& byteBuf) /*throw(InstantiationException, RuntimeException, bad_alloc)*/;

}} // log4cxx::factory
