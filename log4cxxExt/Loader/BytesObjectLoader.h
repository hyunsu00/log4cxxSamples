// BytesObjectLoader.h
//
#pragma once
#include <vector> // std::vector
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr
#include "Exceptions.h" // log4cxx::ext::SmallBufferException, log4cxx::ext::InvalidBufferException

namespace log4cxx { namespace ext { namespace io { namespace Bytes {

	using ByteBuf = std::vector<char>;

	size_t readInt(const ByteBuf& byteBuf, size_t pos, int& value) /*throw(SmallBufferException)*/;
	size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_int64_t& value) /*throw(SmallBufferException)*/;
	size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(SmallBufferException, InvalidBufferException)*/;
	size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(SmallBufferException, InvalidBufferException)*/;

}}}} // log4cxx::ext::io::Bytes

namespace log4cxx { namespace ext { namespace loader { namespace  Bytes {

	using ByteBuf = log4cxx::ext::io::Bytes::ByteBuf;

	// 바이트 프로토콜 반환
	size_t readStart(const ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;
	// LoggingEvent 객체 생성
	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/;

}}}} // log4cxx::ext::loader::Bytes
