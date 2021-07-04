// MsgpackObjectLoader.cpp
#include <iostream>
#include <log4cxx/helpers/charsetdecoder.h> // log4cxx::helpers::CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer
#include <log4cxx/helpers/transcoder.h> // log4cxx::helpers::Transcoder
#include <msgpack.hpp>
#include "LoggingEvnetEx.h"
#include "MsgpackObjectLoader.h"

namespace log4cxx { namespace ext { namespace loader { namespace Msgpack {

	size_t readStart(const ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const size_t pos = 0;
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		// STREAM_MAGIC, STREAM_VERSION
		unsigned char start[] = {
			0xAC, 0xED, 0x00, 0x05
		};
		size_t size = sizeof(start);
		if (pos + readPos + size > byteBufSize) {
			throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		int ret = memcmp(start, pBuf + readPos, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			throw InvalidBufferException(LOG4CXX_STR("시작 데이터가 잘못 되었다."));
		}
		readPos += size;

		return readPos;
	}

	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/
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

		LoggingEventData eventData;
		try {
			msgpack::object_handle oh = ::msgpack::unpack(data, len, readBytes);
			msgpack::object msg = oh.get();
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
			eventData.m_LoggerName = fromUTF8(msgmap.ptr[msgIndex_name].val.as<std::vector<char>>());
			eventData.m_Level = msgmap.ptr[msgIndex_levelno].val.as<int>() * 1000;
			eventData.m_Message = fromUTF8(msgmap.ptr[msgIndex_msg].val.as<std::vector<char>>());
			eventData.m_Timestamp = static_cast<__int64>(std::round(msgmap.ptr[msgIndex_created].val.as<float>() * 1000 * 1000));
			eventData.m_ThreadName = fromUTF8(msgmap.ptr[msgIndex_threadName].val.as<std::vector<char>>());
			eventData.m_PathName = msgmap.ptr[msgIndex_pathname].val.as<std::string>();
			eventData.m_FuncName = msgmap.ptr[msgIndex_funcName].val.as<std::string>();
			eventData.m_LineNumber = std::to_string(msgmap.ptr[msgIndex_lineno].val.as<int>());
		} catch (msgpack::insufficient_bytes& e) {
			LogString logErros;
			log4cxx::helpers::Transcoder::decode(e.what(), logErros);
			throw SmallBufferException(logErros);
		} catch (msgpack::unpack_error& e) {
			LogString logErros;
			log4cxx::helpers::Transcoder::decode(e.what(), logErros);
			throw InvalidBufferException(logErros);
		} catch (...) {
			throw InvalidBufferException(LOG4CXX_STR("data 정보가 잘못 되었다."));
		}
		
		auto event = log4cxx::ext::loader::createLoggingEvent(eventData);
		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + readBytes);
	
		return event;
	}

}}}} // log4cxx::ext::loader::Msgpack
