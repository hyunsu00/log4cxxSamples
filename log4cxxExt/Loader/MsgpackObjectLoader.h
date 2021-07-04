// MsgpackObjectLoader.h
//
#pragma once
#include <vector> // std::vector
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr
#include "Exceptions.h" // log4cxx::ext::SmallBufferException, log4cxx::ext::InvalidBufferException

namespace log4cxx { namespace ext { namespace loader { namespace Msgpack {

	using ByteBuf = std::vector<char>;

	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(msgpack::insufficient_bytes, msgpack::unpack_error)*/;

}}}} // log4cxx::ext::loader::Msgpack
