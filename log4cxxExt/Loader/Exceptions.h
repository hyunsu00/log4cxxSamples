// Exceptions.h
//
#pragma once
#include <log4cxx/helpers/exception.h> // log4cxx::helpers::RuntimeException

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
