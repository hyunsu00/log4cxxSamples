// log4cxxClient.cpp
//
#include "log4cxxSocket.h"
#include "Exceptions.h"
#include "log4cxxClient.h"

namespace log4cxx { namespace ext { namespace socket {

    static log4cxx::LoggerPtr s_Logger = log4cxx::Logger::getRootLogger();
    void Client::setLogger(const std::string& name)
    {
        s_Logger = log4cxx::Logger::getLogger(name);
    }

    Client::Client(SOCKET socket)
    : m_Socket(socket)
    , m_LoggingFunc(nullptr)
    , m_ByteBuf()
    {
    }

    Client::~Client()
    {
    }

    bool Client::operator<(const Client& other) const 
    {
        return m_Socket < other.m_Socket;
    }

    bool Client::operator==(const Client& other) const
    {
        return m_Socket == other.m_Socket;
    }

    Client::operator SOCKET() const 
    {
        return m_Socket;
    }

    bool Client::forceLog(const char* bytes, size_t size) const
    {
        m_ByteBuf.insert(m_ByteBuf.end(), bytes, bytes + size);

        if (!m_LoggingFunc) {
            try {
                m_LoggingFunc = log4cxx::ext::loader::createLoggingEventFunc(m_ByteBuf);
            } catch (log4cxx::ext::SmallBufferException& e) { // 무시
                LOG4CXX_WARN(s_Logger, e.what());
                return true;
            } catch (...) {
                LOG4CXX_ERROR(s_Logger, LOG4CXX_STR("loggingEventFunc 생성 실패."));
                return false;
            }
            
            if (!m_LoggingFunc) {
                LOG4CXX_ERROR(s_Logger, LOG4CXX_STR("loggingEventFunc 생성 실패."));
                return false;
            }
        }

        while (!m_ByteBuf.empty()) {
            try {
                log4cxx::spi::LoggingEventPtr event = m_LoggingFunc(m_ByteBuf);
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

        return true;
    }

}}} // log4cxx::ext::socket
