// log4cxxClient.cpp
#include <string>
#include <vector>
#include <log4cxx/log4cxx.h>
#include "log4cxxSocket.h"
#include "ByteBufInputStream.h"
#include "ObjectLoader.h"
#include "log4cxxClient.h"

namespace log4cxx { namespace ext { namespace socket {

    static log4cxx::LoggerPtr s_Logger = log4cxx::Logger::getRootLogger();
    void Client::setLogger(const std::string& name)
    {
        s_Logger = log4cxx::Logger::getLogger(name);
    }

    Client::Client(SOCKET socket)
    : m_Socket(socket)
    , m_Start(true)
    , m_ByteBuf()
    {
    }
    bool Client::operator<(const Client& other) const {
        return m_Socket < other.m_Socket;
    }
    Client::operator SOCKET() const {
        return m_Socket;
    }

    bool Client::forceLog(const char* bytes, size_t size) const
    {
        m_ByteBuf.insert(m_ByteBuf.end(), bytes, bytes + size);
        if (m_Start) {
            size_t readBytes = 0;
            try {
                readBytes = log4cxx::ext::loader::readStart(m_ByteBuf);
            } catch (log4cxx::ext::SmallBufferException& e) { // 무시
                LOG4CXX_WARN(s_Logger, e.what());
                return true;
            } catch (log4cxx::ext::InvalidBufferException& e) { // 종료
                LOG4CXX_ERROR(s_Logger, e.what());
                return false;
            }
            m_ByteBuf.erase(m_ByteBuf.begin(), m_ByteBuf.begin() + readBytes);

            m_Start = false;
        }

        if (!m_Start) {
            while (!m_ByteBuf.empty()) {
                try {
                    log4cxx::spi::LoggingEventPtr event = log4cxx::ext::loader::createLoggingEvent(m_ByteBuf);
                    log4cxx::LoggerPtr remoteLogger = log4cxx::Logger::getLogger(event->getLoggerName());
                    if (event->getLevel()->isGreaterOrEqual(remoteLogger->getEffectiveLevel())) {
                        log4cxx::helpers::Pool p;
                        remoteLogger->callAppenders(event, p);
                    }
                } catch (log4cxx::ext::SmallBufferException& e) { // 무시
                    LOG4CXX_WARN(s_Logger, e.what());
                    break;
                } catch (log4cxx::ext::InvalidBufferException& e) { // 종료
                    LOG4CXX_ERROR(s_Logger, e.what());
                    return false;
                }
            }
        }

        return true;
    }

}}} // log4cxx::ext::socket
