// ObjectLoader.h
#pragma once
#include <vector> // std::vector
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr

namespace log4cxx { namespace ext { namespace io {

    using ByteBuf = std::vector<char>;
    ByteBuf loadFile(const char* filename);

}}} // log4cxx::ext::io

namespace log4cxx { namespace ext { namespace loader {

    using ByteBuf = io::ByteBuf;

    // 자바 스트림 프로토콜 반환
    size_t readStart(const ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;
    bool readStart(int socket) noexcept;

    // LoggingEvent 객체 생성
    log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;
    log4cxx::spi::LoggingEventPtr createLoggingEvent(SOCKET socket) noexcept;

}}} // log4cxx::ext::loader
