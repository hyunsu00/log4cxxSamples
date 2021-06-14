// byteBufInputStream.cpp
#include <log4cxx/log4cxx.h>
#include "inputStreamDef.h"
#include "byteBufInputStream.h"
#include <log4cxx/helpers/exception.h> // SmallBufferException, InvalidBufferException
#include <log4cxx/helpers/charsetdecoder.h> // log4cxx::helpers::CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer
#include <memory> // std::unique_ptr
#include <memory.h> // memcmp

#ifdef _WIN32
#	include <crtdbg.h>
#else
#	include <assert.h>
#	define _ASSERTE assert
#endif

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

namespace log4cxx { namespace ext { namespace io {

	size_t readByte(const ByteBuf& byteBuf, size_t pos, unsigned char& value) /*throw(SmallBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		_ASSERTE(sizeof(unsigned char) == 1 && "읽을 데이터의 크기는 1이여야 한다.");
		const size_t size = 1;
		if (pos + readPos + size > byteBufSize) {
			throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		memcpy(&value, pBuf + readPos, size);
		readPos += size;

		return readPos;
	}

	size_t readBytes(const ByteBuf& byteBuf, size_t pos, size_t bytes, std::vector<unsigned char>& value) /*throw(SmallBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		const size_t size = bytes;
		if (pos + readPos + size > byteBufSize) {
			throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		value = std::vector<unsigned char>(bytes, 0);
		memcpy(&value[0], pBuf + readPos, size);
		readPos += size;

		return readPos;
	}

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

	size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_time_t& value) /*throw(SmallBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		_ASSERTE(sizeof(log4cxx_time_t) == 8 && "읽을 데이터의 크기는 8이여야 한다.");
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

	size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value, bool skipTypeClass /*= false*/) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		if (!skipTypeClass) {
			unsigned char typeClass = io::TC_STRING;
			size_t size = io::readByte(byteBuf, pos, typeClass);
			if (typeClass != io::TC_STRING) {
				throw InvalidBufferException(LOG4CXX_STR("type이 TC_STRING이여야만 한다."));
			}
			readPos += size;
		}

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		{
			char bytes[2];
			const size_t size = sizeof(bytes);
			if (pos + readPos + size > byteBufSize) {
				throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}

			// 빅 엔디안
			memcpy(bytes, pBuf + readPos, size);

			// 리틀 엔디안으로 변경
			std::swap(bytes[0], bytes[1]);
			memcpy(&dataLen, bytes, sizeof(bytes));

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

	size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value, bool skipTypeClass /*= false*/) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		if (!skipTypeClass) {
			unsigned char typeClass = io::TC_STRING;
			size_t size = io::readByte(byteBuf, pos, typeClass);
			if (typeClass != io::TC_STRING) {
				throw InvalidBufferException(LOG4CXX_STR("type이 TC_STRING이여야만 한다."));
			}
			readPos += size;
		}

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		{
			char bytes[2];
			const size_t size = sizeof(bytes);
			if (pos + readPos + size > byteBufSize) {
				throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}

			// 빅 엔디안
			memcpy(bytes, pBuf + readPos, size);

			// 리틀 엔디안으로 변경
			std::swap(bytes[0], bytes[1]);
			memcpy(&dataLen, bytes, sizeof(bytes));

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

	size_t readObject(const ByteBuf& byteBuf, size_t pos, MDC::Map& value, bool skipTypeClass/* = false*/) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		if (!skipTypeClass) {
			unsigned char typeClass = io::TC_OBJECT;
			size_t size = io::readByte(byteBuf, pos, typeClass);
			if (typeClass != io::TC_OBJECT) {
				throw InvalidBufferException(LOG4CXX_STR("type이 TC_OBJECT이여야만 한다."));
			}
			readPos += size;
		}

		std::pair<std::string, unsigned int> prolog;
		size_t size = readProlog(byteBuf, pos, classDesc::HASH_TABLE, sizeof(classDesc::HASH_TABLE), prolog);
		readPos += size;

		// == os->write(dataBuf, p);
		{
			const char data[] = {
				0x3F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
				TC_BLOCKDATA, 0x08, 0x00, 0x00, 0x00, 0x07
			};
			size = sizeof(data);
			if (pos + readPos + size > byteBufSize) {
				throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}

			int ret = memcmp(data, pBuf + readPos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				throw InvalidBufferException(LOG4CXX_STR("data 정보가 잘못 되었다."));
			}
			readPos += size;
		}
		
		// == os->write(sizeBuf, p);
		{
			int mapSize = 0;
			size = readInt(byteBuf, pos + readPos, mapSize);
			readPos += size;

			for (int i = 0; i < mapSize; i++) {
				LogString first, second;
				// <==> writeObject(iter->first, p);
				size = readLogString(byteBuf, pos + readPos, first);
				readPos += size;

				// <==> writeObject(iter->second, p);
				size = readLogString(byteBuf, pos + readPos, second);
				readPos += size;

				value[first] = second;
			}
		}
		
		// == writeByte(TC_ENDBLOCKDATA, p);
		{
			unsigned char value = TC_ENDBLOCKDATA;
			size = readByte(byteBuf, pos + readPos, value);
			if (value != TC_ENDBLOCKDATA) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_ENDBLOCKDATA 여야만 한다."));
			}
			readPos += size;
		}

		return readPos;
	}

	size_t readProlog(
		const ByteBuf& byteBuf,
		size_t pos,
		const unsigned char* classDesc,
		size_t classDescLen,
		std::pair<std::string, unsigned int>& value
	) /*throw(SmallBufferException, InvalidBufferException)*/ {
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		unsigned char typeClass = io::TC_CLASSDESC;
		size_t size = io::readByte(byteBuf, pos + readPos, typeClass);
		readPos += size;

		switch (typeClass)
		{
		case TC_CLASSDESC:
			{
				size = classDescLen - sizeof(classDesc[0]);
				if (pos + readPos + size > byteBufSize) {
					throw SmallBufferException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
				}

				int ret = memcmp(classDesc + 1, pBuf + readPos, size);
				_ASSERTE(ret == 0 && "memcmp() Failed");
				if (ret != 0) {
					throw InvalidBufferException(LOG4CXX_STR("TC_CLASSDESC의 클래스 정보가 잘못되었다."));
				}
				readPos += size;
			}
			break;
		case TC_REFERENCE:
			{
				int val = 0;
				size = io::readInt(byteBuf, pos + readPos, val);
				readPos += size;

				value.second = val;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			throw InvalidBufferException(LOG4CXX_STR("type 정보를 처리 할수 없다."));
		}

		return readPos;
	}

	size_t readLocationInfo(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		unsigned char typeClass = io::TC_NULL;
		size_t size = io::readByte(byteBuf, pos + readPos, typeClass);
		readPos += size;

		switch (typeClass)
		{
		case TC_NULL:
			{
				value.clear();
			}
			break;
		case TC_OBJECT:
			{
				std::pair<std::string, unsigned int> prolog;
				size = readProlog(byteBuf, pos + readPos, classDesc::LOCATION_INFO, sizeof(classDesc::LOCATION_INFO), prolog);
				readPos += size;

				std::string fullInfo;
				size = readUTFString(byteBuf, pos + readPos, fullInfo);
				readPos += size;

				value = fullInfo;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			throw InvalidBufferException(LOG4CXX_STR("type 정보를 처리 할수 없다."));
		}
		
		return readPos;
	}

	size_t readMDC(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		unsigned char typeClass = io::TC_NULL;
		size_t size = io::readByte(byteBuf, pos + readPos, typeClass);
		readPos += size;

		switch (typeClass)
		{
		case TC_NULL:
			{
				value.clear();
			}
			break;
		case TC_OBJECT:
			{
				MDC::Map mdcMap;
				size = readObject(byteBuf, pos + readPos, mdcMap, true);
				readPos += size;

				value = mdcMap;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			throw InvalidBufferException(LOG4CXX_STR("type 정보를 처리 할수 없다."));
		}
		
		return readPos;
	}

	size_t readNDC(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readPos = 0;

		unsigned char typeClass = io::TC_NULL;
		size_t size = io::readByte(byteBuf, pos + readPos, typeClass);
		readPos += size;

		switch (typeClass)
		{
		case TC_NULL:
			{
				value.clear();
			}
			break;
		case TC_STRING:
			{
				LogString ndc;
				size = readLogString(byteBuf, pos + readPos, ndc, true);
				readPos += size;

				value = ndc;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			return false;
		}
		
		return readPos;
	}

}}} // // log4cxx::ext::io
