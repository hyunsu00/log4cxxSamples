// DefaultObjectLoader.h
#pragma once

#include <vector> // std::vector
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr
#include "Exceptions.h" // log4cxx::ext::SmallBufferException, log4cxx::ext::InvalidBufferException

namespace log4cxx { namespace ext { namespace loader { namespace Default {

    using ByteBuf = std::vector<char>;

    // 자바 스트림 프로토콜 반환
    size_t readStart(const ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;
    bool readStart(SOCKET socket) noexcept;

    // LoggingEvent 객체 생성
    log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;
    log4cxx::spi::LoggingEventPtr createLoggingEvent(SOCKET socket) noexcept;

}}}} // log4cxx::ext::loader::Default
