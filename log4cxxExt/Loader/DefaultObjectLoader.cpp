// DefaultObjectLoader.cpp
//
#ifdef _WIN32
#	include <winsock2.h> // SOCKET
#	include <crtdbg.h> // _ASSERTE
#else
typedef int SOCKET;
#	include <assert.h> // assert
#	define _ASSERTE assert
#endif
#include <memory.h> // memcpy

#include "DefaultInputStreamDef.h"
#include "DefaultByteBufInputStream.h"
#include "DefaultSocketInputStream.h"
#include "LoggingEvnetEx.h"
#include "DefaultObjectLoader.h"

namespace log4cxx { namespace ext { namespace loader { namespace Default {
	
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

	bool readStart(SOCKET socket) noexcept
	{
		using namespace log4cxx::ext::io::Default;

		// STREAM_MAGIC, STREAM_VERSION
		unsigned char start[] = {
			0xAC, 0xED, 0x00, 0x05
		};
		size_t size = sizeof(start);
		std::vector<unsigned char> byteBuf;
		if (!readBytes(socket, size, byteBuf)) {
			return false;
		}
		int ret = memcmp(start, &byteBuf[0], size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return false;
		}

		return true;
	}

	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		using namespace log4cxx::ext::io::Default;

		const char* pBuf = &byteBuf[0];
		size_t pos = 0;

		// <==> writeProlog(os, p);
		{
			unsigned char typeClass = TC_OBJECT;
			size_t size = readByte(byteBuf, pos, typeClass);
			if (typeClass != TC_OBJECT) {
				throw InvalidBufferException(LOG4CXX_STR("type이 TC_OBJECT이여야만 한다."));
			}
			pos += size;

			std::pair<std::string, unsigned int> value;
			size = readProlog(byteBuf, pos, classDesc::LOGGING_EVENT, sizeof(classDesc::LOGGING_EVENT), value);
			pos += size;
		}

		// <==> os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			std::vector<unsigned char> value;
			size_t size = readBytes(byteBuf, pos, sizeof(lookupsRequired), value);
			int ret = memcmp(lookupsRequired, &value[0], sizeof(lookupsRequired));
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				throw InvalidBufferException(LOG4CXX_STR("lookupsRequired와 value는 같아야 한다."));
			}
			pos += size;
		}

		// <==> os.writeLong(timeStamp/1000, p);
		log4cxx_time_t timeStamp = 0;
		{
			size_t size = readLong(byteBuf, pos, timeStamp);
			timeStamp *= 1000;
			pos += size;
		}

		// <==> os.writeObject(logger, p);
		LogString logger;
		{
			size_t size = readLogString(byteBuf, pos, logger);
			pos += size;
		}

		// <==> locationInfo.write(os, p);
		std::string fullInfo;
		{
			size_t size = readLocationInfo(byteBuf, pos, fullInfo);
			pos += size;
		}

		// <==> mdc
		MDC::Map mdcMap;
		{
			size_t size = readMDC(byteBuf, pos, mdcMap);
			pos += size;
		}

		// <==> ndc 
		LogString ndc;
		{
			size_t size = readNDC(byteBuf, pos, ndc);
			pos += size;
		}

		// <==> os.writeObject(message, p);
		LogString message;
		{
			size_t size = readLogString(byteBuf, pos, message);
			pos += size;
		}

		// <==> os.writeObject(threadName, p);
		LogString threadName;
		{
			size_t size = readLogString(byteBuf, pos, threadName);
			pos += size;
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = TC_NULL;
			size_t size = readByte(byteBuf, pos, value);
			if (value != TC_NULL) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			unsigned char value = TC_BLOCKDATA;
			size_t size = readByte(byteBuf, pos, value);
			if (value != TC_BLOCKDATA) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_BLOCKDATA 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(0x04, p);
		{
			unsigned char value = 0x04;
			size_t size = readByte(byteBuf, pos, value);
			if (value != 0x04) {
				throw InvalidBufferException(LOG4CXX_STR("value는 0x04 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeInt(level->toInt(), p);
		int level = 0;
		{
			size_t size = readInt(byteBuf, pos, level);
			pos += size;
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = TC_NULL;
			size_t size = readByte(byteBuf, pos, value);
			if (value != TC_NULL) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			unsigned char value = TC_ENDBLOCKDATA;
			size_t size = readByte(byteBuf, pos, value);
			if (value != TC_ENDBLOCKDATA) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		size_t readBytes = pos;
		_ASSERTE((byteBuf.size() >= readBytes) && "모든 값을 읽지 못했다.");
		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + readBytes);

		auto eventPtr = log4cxx::ext::loader::createLoggingEvent(
			logger, 
			Level::toLevel(level), 
			message, 
			timeStamp, 
			threadName, 
			fullInfo
		);

		return eventPtr;
	}

	log4cxx::spi::LoggingEventPtr createLoggingEvent(SOCKET socket) noexcept
	{
		using namespace log4cxx::ext::io::Default;

		// <==> writeProlog(os, p);
		{
			unsigned char typeClass = TC_OBJECT;
			if (!readByte(socket, typeClass) || typeClass != TC_OBJECT) {
				return nullptr;
			}

			std::pair<std::string, unsigned int> value;
			if (!readProlog(socket, classDesc::LOGGING_EVENT, sizeof(classDesc::LOGGING_EVENT), value)) {
				return nullptr;
			}
		}

		// <==> os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			std::vector<unsigned char> value;
			if (!readBytes(socket, sizeof(lookupsRequired), value)) {
				return nullptr;
			}
			int ret = memcmp(lookupsRequired, &value[0], sizeof(lookupsRequired));
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return nullptr;
			}
		}

		// <==> os.writeLong(timeStamp/1000, p);
		log4cxx_time_t timeStamp = 0;
		{
			if (!readLong(socket, timeStamp)) {
				return nullptr;
			}
			timeStamp *= 1000;
		}

		// <==> os.writeObject(logger, p);
		LogString logger;
		{
			if (!readLogString(socket, logger)) {
				return nullptr;
			}
		}

		// <==> locationInfo.write(os, p);
		std::string fullInfo;
		{
			if (!readLocationInfo(socket, fullInfo)) {
				return nullptr;
			}
		}

		// <==> mdc
		MDC::Map mdcMap;
		{
			if (!readMDC(socket, mdcMap)) {
				return nullptr;
			}
		}

		// <==> ndc 
		LogString ndc;
		{
			if (!readNDC(socket, ndc)) {
				return nullptr;
			}
		}

		// <==> os.writeObject(message, p);
		LogString message;
		{
			if (!readLogString(socket, message)) {
				return nullptr;
			}
		}

		// <==> os.writeObject(threadName, p);
		LogString threadName;
		{
			if (!readLogString(socket, threadName)) {
				return nullptr;
			}
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = TC_NULL;
			if (!readByte(socket, value) || value != TC_NULL) {
				return nullptr;
			}
		}

		// <==> os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			unsigned char value = TC_BLOCKDATA;
			if (!readByte(socket, value) || value != TC_BLOCKDATA) {
				return nullptr;
			}
		}

		// <==> os.writeByte(0x04, p);
		{
			unsigned char value = 0x04;
			if (!readByte(socket, value) || value != 0x04) {
				return nullptr;
			}
		}

		// <==> os.writeInt(level->toInt(), p);
		int level = 0;
		{
			if (!readInt(socket, level)) {
				return nullptr;
			}
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = TC_NULL;
			if (!readByte(socket, value) || value != TC_NULL) {
				return nullptr;
			}
		}

		// <==> os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			unsigned char value = TC_ENDBLOCKDATA;
			if (!readByte(socket, value) || value != TC_ENDBLOCKDATA) {
				return nullptr;
			}
		}

		auto eventPtr = log4cxx::ext::loader::createLoggingEvent(
			logger,
			Level::toLevel(level),
			message,
			timeStamp,
			threadName,
			fullInfo
		);

		return eventPtr;
	}

}}}} // log4cxx::ext::loader::Default
