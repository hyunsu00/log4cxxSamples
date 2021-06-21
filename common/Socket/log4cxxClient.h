// log4cxxClient.h
//
#pragma once

namespace log4cxx { namespace ext { namespace socket {

    class Client
    {
    public:
        static void setLogger(const std::string& logger);

    public:
        Client(SOCKET socket);
        ~Client();
        bool operator<(const Client& other) const;
        operator SOCKET() const;

    public:
        // 복사 생성자 / 대입연산자는 막는다.
        Client(const Client& other) = delete;
        Client& operator=(const Client& other) = delete;

    public:
        bool forceLog(const char* bytes, size_t size) const;

    private:
        SOCKET m_Socket;
        mutable bool m_Start;
        mutable std::vector<char> m_ByteBuf;
    }; // class Client 

}}} // log4cxx::ext::socket