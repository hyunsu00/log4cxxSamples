// log4cxxClient.h
//
#pragma once
#include <string> // std::string
#include <vector> // std::vector
#include <ObjectLoader.h>

namespace log4cxx { namespace ext { namespace socket {

    class Client
    {
    public:
        static void setLogger(const std::string& logger);

    public:
        Client(SOCKET socket);
        ~Client();
        bool operator<(const Client& other) const; // std::set
        bool operator==(const Client& other) const; // std::unordered_set
        operator SOCKET() const;

    public:
        // 복사 생성자 / 대입연산자는 막는다.
        Client(const Client& other) = delete;
        Client& operator=(const Client& other) = delete;

    public:
        bool forceLog(const char* bytes, size_t size) const;

    private:
        SOCKET m_Socket;
        mutable loader::LoggingEventFunc m_LoggingFunc;
        mutable std::vector<char> m_ByteBuf;
    }; // class Client 

}}} // log4cxx::ext::socket

namespace std {
    template <>
    struct hash<log4cxx::ext::socket::Client> {
        size_t operator()(const log4cxx::ext::socket::Client& client) const {
            return hash<SOCKET>()(client);
        }
    };
}  // namespace std
