// MsgpackObjectLoader.h
//
#pragma once

namespace log4cxx { namespace ext { namespace loader { namespace msgpack {

	using ByteBuf = std::vector<char>;

	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(msgpack::insufficient_bytes, msgpack::unpack_error)*/;

}}}} // log4cxx::ext::loader::msgpack
