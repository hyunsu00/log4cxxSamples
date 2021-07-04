// BytesObjectLoader.cpp
//
#ifdef _WIN32
#	include <crtdbg.h> // _ASSERTE
#else
#	include <assert.h> // assert
#	define _ASSERTE assert
#endif
#include <memory.h> // memcpy

#include <log4cxx/helpers/charsetdecoder.h> // log4cxx::helpers::CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer
#include "Exceptions.h"
#include "LoggingEvnetEx.h"
#include "BytesObjectLoader.h"

namespace log4cxx { namespace ext { namespace io { namespace Bytes {

	size_t readInt(const ByteBuf& byteBuf, size_t pos, int& value) /*throw(SmallBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		_ASSERTE(sizeof(int) == 4 && "읽을 데이터의 크기는 4이여야 한다.");
		const size_t size = 4;
		if (pos + readPos + size > byteBufSize) {
			throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		// 빅 엔디안
		memcpy(&value, pBuf + readPos, size);

		// 리틀 엔디안으로 변경
		char bytes[4] = { 0, };
		bytes[3] = (char)(value & 0xFF);
		bytes[2] = (char)((value >> 8) & 0xFF);
		bytes[1] = (char)((value >> 16) & 0xFF);
		bytes[0] = (char)((value >> 24) & 0xFF);

		memcpy(&value, bytes, sizeof(bytes));
		// == 
/*
		char bytes[4];
		// 빅 엔디안
		memcpy(bytes, pBuf + readPos, size);

		// 리틀 엔디안으로 변경
		std::swap(bytes[0], bytes[3]);
		std::swap(bytes[1], bytes[2]);

		memcpy(&value, bytes, sizeof(bytes));
*/
		readPos += size;
		return readPos;
	}

	size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_int64_t& value) /*throw(SmallBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		_ASSERTE(sizeof(log4cxx_int64_t) == 8 && "읽을 데이터의 크기는 8이여야 한다.");
		const size_t size = 8;
		if (pos + readPos + size > byteBufSize) {
			throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		// 빅 엔디안
		memcpy(&value, pBuf + readPos, size);

		// 리틀 엔디안으로 변경
		char bytes[8] = { 0, };
		bytes[7] = (char)(value & 0xFF);
		bytes[6] = (char)((value >> 8) & 0xFF);
		bytes[5] = (char)((value >> 16) & 0xFF);
		bytes[4] = (char)((value >> 24) & 0xFF);
		bytes[3] = (char)((value >> 32) & 0xFF);
		bytes[2] = (char)((value >> 40) & 0xFF);
		bytes[1] = (char)((value >> 48) & 0xFF);
		bytes[0] = (char)((value >> 56) & 0xFF);

		memcpy(&value, bytes, sizeof(bytes));
		// == 
/*
		char bytes[8];
		// 빅 엔디안
		memcpy(bytes, pBuf + readSize, size);

		// 리틀 엔디안으로 변경
		std::swap(bytes[0], bytes[7]);
		std::swap(bytes[1], bytes[6]);
		std::swap(bytes[2], bytes[5]);
		std::swap(bytes[3], bytes[4]);

		memcpy(&value, bytes, sizeof(bytes));
*/
		readPos += size;
		return readPos;
	}

	size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		{
			const size_t size = readInt(byteBuf, pos + readPos, (int&)dataLen);
			readPos += size;
		}

		// UTF 스트링 복사
		std::vector<char> data(dataLen, 0);
		{
			const size_t size = data.size();
			if (pos + readPos + size > byteBufSize) {
				throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}
			memcpy(&data[0], pBuf + readPos, size);

			readPos += size;
		}

		// 가장뒤에 0 추가
		data.push_back(0);
		value = &data[0];

		return readPos;
	}

	size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		{
			const size_t size = readInt(byteBuf, pos + readPos, (int&)dataLen);
			readPos += size;
		}

		// UTF 스트링 복사
		std::vector<char> data(dataLen, 0);
		{
			const size_t size = data.size();
			if (pos + readPos + size > byteBufSize) {
				throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}
			memcpy(&data[0], pBuf + readPos, size);

			readPos += size;
		}

		// UTF-8 스트링 -> LogString으로 변환
		{
			log4cxx::helpers::CharsetDecoderPtr utf8Decoder(log4cxx::helpers::CharsetDecoder::getUTF8Decoder());
			log4cxx::helpers::ByteBuffer buf(&data[0], data.size());
			utf8Decoder->decode(buf, value);
		}

		return readPos;
	}

}}}} // log4cxx::ext::io::Bytes

namespace log4cxx { namespace ext { namespace loader { namespace  Bytes {

	size_t readStart(const ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const size_t pos = 0;
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		// STREAM_MAGIC, STREAM_VERSION
		unsigned char start[] = {
			0xAC, 0xED, 0x01, 0x05
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
		const char* pBuf = &byteBuf[0];
		size_t pos = 0;

		LoggingEventData eventData;
		pos += io::Bytes::readLogString(byteBuf, pos, eventData.m_LoggerName);
		pos += io::Bytes::readInt(byteBuf, pos, eventData.m_Level);
		pos += io::Bytes::readLogString(byteBuf, pos, eventData.m_Message);
		pos += io::Bytes::readLong(byteBuf, pos, eventData.m_Timestamp);
		pos += io::Bytes::readLogString(byteBuf, pos, eventData.m_ThreadName);
		pos += io::Bytes::readUTFString(byteBuf, pos, eventData.m_PathName);
		pos += io::Bytes::readUTFString(byteBuf, pos, eventData.m_FuncName);
		pos += io::Bytes::readUTFString(byteBuf, pos, eventData.m_LineNumber);

		size_t readBytes = pos;
		_ASSERTE((byteBuf.size() >= readBytes) && "모든 값을 읽지 못했다.");
		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + readBytes);

		return log4cxx::ext::loader::createLoggingEvent(eventData);
	}

}}}} // log4cxx::ext::loader::Bytes
