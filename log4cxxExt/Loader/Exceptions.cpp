// Exceptions.cpp
//
#include "Exceptions.h"

namespace log4cxx { namespace ext {

	SmallBufferException::SmallBufferException(const LogString& msg1)
	: RuntimeException(msg1)
	{
	}

	SmallBufferException::SmallBufferException(const SmallBufferException& src)
	: RuntimeException(src)
	{
	}

	SmallBufferException& SmallBufferException::operator=(const SmallBufferException& src)
	{
		RuntimeException::operator=(src);
		return *this;
	}

	InvalidBufferException::InvalidBufferException(const LogString& msg1)
	: RuntimeException(msg1)
	{
	}

	InvalidBufferException::InvalidBufferException(const InvalidBufferException& src)
	: RuntimeException(src)
	{
	}

	InvalidBufferException& InvalidBufferException::operator=(const InvalidBufferException& src)
	{
		RuntimeException::operator=(src);
		return *this;
	}

}} // log4cxx::ext
