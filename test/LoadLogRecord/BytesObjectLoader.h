// BytesObjectLoader.h
//
#pragma once

namespace log4cxx { namespace ext { namespace loader { namespace  bytes {

	using ByteBuf = std::vector<char>;

	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf);

}}}} // log4cxx::ext::loader::bytes
