// ObjectLoader.h
//
#pragma once
#include <vector> // std::vector
#include <functional> // std::function
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr

namespace log4cxx { namespace ext { namespace loader {

    using ByteBuf = std::vector<char>;
    using StartFunc = std::function<size_t(const ByteBuf&)>;
    using LoggingEventFunc = std::function<log4cxx::spi::LoggingEventPtr(ByteBuf&)>;
    LoggingEventFunc createLoggingEventFunc(ByteBuf& byteBuf);

}}} // log4cxx::ext::loader
