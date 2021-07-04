// ObjectLoader.cpp
//
#ifdef _WIN32
#	include <winsock2.h> // SOCKET
#else
typedef int SOCKET;
#endif
#include "ObjectLoader.h"
#include "DefaultObjectLoader.h"
#include "BytesObjectLoader.h"
#include "MsgpackObjectLoader.h"

namespace log4cxx { namespace ext { namespace loader {

    static auto readDefaultStart = [](const ByteBuf& byteBuf) -> size_t {
        return log4cxx::ext::loader::Default::readStart(byteBuf);
    };
    static auto createDefaultLoggingEvent = [](ByteBuf& byteBuf)-> log4cxx::spi::LoggingEventPtr {
        return log4cxx::ext::loader::Default::createLoggingEvent(byteBuf);
    };
    static auto readBytesStart = [](const ByteBuf& byteBuf) -> size_t {
        return log4cxx::ext::loader::Bytes::readStart(byteBuf);
    };
    static auto createBytesLoggingEvent = [](ByteBuf& byteBuf)-> log4cxx::spi::LoggingEventPtr {
        return log4cxx::ext::loader::Bytes::createLoggingEvent(byteBuf);
    };
    static auto readMsgpackStart = [](const ByteBuf& byteBuf) -> size_t {
        return log4cxx::ext::loader::Msgpack::readStart(byteBuf);
    };
    static auto createMsgpackLoggingEvent = [](ByteBuf& byteBuf)-> log4cxx::spi::LoggingEventPtr {
        return log4cxx::ext::loader::Msgpack::createLoggingEvent(byteBuf);
    };

    static std::vector<std::pair<StartFunc, LoggingEventFunc>> sloggingEventFuncs = {
        {readDefaultStart, createDefaultLoggingEvent},
        {readBytesStart, createBytesLoggingEvent},
        {readMsgpackStart, createMsgpackLoggingEvent}
    };

    LoggingEventFunc createLoggingEventFunc(ByteBuf& byteBuf)
    {
        for (auto loggingEvnetFunc : sloggingEventFuncs) {
            try {
                size_t readBytes = loggingEvnetFunc.first(byteBuf);
                byteBuf.erase(byteBuf.begin(), byteBuf.begin() + readBytes);
                return loggingEvnetFunc.second;
            } catch (log4cxx::ext::SmallBufferException& /*e*/) { // 무시
                throw;
            } catch (log4cxx::ext::InvalidBufferException& /*e*/) {
                continue;
            } catch (...) {
                throw;
            }
        }

        return nullptr;
    }

}}} // log4cxx::ext::loader