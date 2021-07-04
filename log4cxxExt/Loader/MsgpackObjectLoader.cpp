// MsgpackObjectLoader.cpp
#include <iostream>
#include <log4cxx/helpers/charsetdecoder.h> // log4cxx::helpers::CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer
#include <msgpack.hpp>
#include "LoggingEvnetEx.h"
#include "MsgpackObjectLoader.h"

namespace log4cxx { namespace ext { namespace loader { namespace Msgpack {

	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(msgpack::insufficient_bytes, msgpack::unpack_error)*/
	{
		auto fromUTF8 = [](const std::vector<char>& data) -> LogString {
			LogString value;
			log4cxx::helpers::CharsetDecoderPtr utf8Decoder(log4cxx::helpers::CharsetDecoder::getUTF8Decoder());
			log4cxx::helpers::ByteBuffer buf(const_cast<char*>(&data[0]), data.size());
			utf8Decoder->decode(buf, value);

			return value;
		};

		enum msgIndex {
			msgIndex_name = 0,
			msgIndex_levelno = 4,
			msgIndex_msg = 1,
			msgIndex_created = 13,
			msgIndex_threadName = 17,
			msgIndex_pathname = 5,
			msgIndex_funcName = 12,
			msgIndex_lineno = 11
		};

		const char* data = &byteBuf[0];
		size_t len = byteBuf.size();
		std::size_t readBytes = 0;
		::msgpack::object_handle oh = ::msgpack::unpack(data, len, readBytes);
		::msgpack::object msg = oh.get();
		_ASSERTE(msg.type == ::msgpack::type::MAP && "msgpack 루트타입은 map이여야 한다.");

		auto const& msgmap = msg.via.map;
#if 0
		{
			std::cout << msgmap.ptr[msgIndex_name].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_name].val.as<std::string>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_levelno].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_levelno].val.as<int>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_msg].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_msg].val.as<std::string>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_created].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_created].val.as<float>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_threadName].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_threadName].val.as<std::string>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_pathname].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_pathname].val.as<std::string>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_funcName].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_funcName].val.as<std::string>() << std::endl;

			std::cout << msgmap.ptr[msgIndex_lineno].key.as<std::string>() << " : ";
			std::cout << msgmap.ptr[msgIndex_lineno].val.as<int>() << std::endl;
		}
#endif

		LoggingEventData eventData;
		eventData.m_LoggerName = fromUTF8(msgmap.ptr[msgIndex_name].val.as<std::vector<char>>());
		eventData.m_Level = msgmap.ptr[msgIndex_levelno].val.as<int>() * 1000;
		eventData.m_Message = fromUTF8(msgmap.ptr[msgIndex_msg].val.as<std::vector<char>>());
		eventData.m_Timestamp = static_cast<__int64>(std::round(msgmap.ptr[msgIndex_created].val.as<float>() * 1000 * 1000));
		eventData.m_ThreadName = fromUTF8(msgmap.ptr[msgIndex_threadName].val.as<std::vector<char>>());
		eventData.m_PathName = msgmap.ptr[msgIndex_pathname].val.as<std::string>();
		eventData.m_FuncName = msgmap.ptr[msgIndex_funcName].val.as<std::string>();
		eventData.m_LineNumber = std::to_string(msgmap.ptr[msgIndex_lineno].val.as<int>());

		auto event = log4cxx::ext::loader::createLoggingEvent(eventData);
		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + readBytes);
	
		return event;
	}

}}}} // log4cxx::ext::loader::Msgpack
