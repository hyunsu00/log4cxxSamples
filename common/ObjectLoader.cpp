// ObjectLoader.cpp
#include "ObjectLoader.h"
#include <log4cxx/helpers/exception.h> // helpers::InstantiationException, helpers::RuntimeException
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

// https://stackoverflow.com/questions/424104/can-i-access-private-members-from-outside-the-class-without-using-friends
namespace {

	#define CONCATE_(X, Y) X##Y
	#define CONCATE(X, Y) CONCATE_(X, Y)

	#define ALLOW_ACCESS(CLASS, MEMBER, ...) \
	  template<typename T, __VA_ARGS__ CLASS::*Member> \
	  struct CONCATE(MEMBER, __LINE__) { friend __VA_ARGS__ CLASS::*Access(T*) { return Member; } }; \
	  template<typename> struct T_##MEMBER; \
	  template<> struct T_##MEMBER<CLASS> { friend __VA_ARGS__ CLASS::*Access(T_##MEMBER<CLASS>*); }; \
	  template struct CONCATE(MEMBER, __LINE__)<T_##MEMBER<CLASS>, &CLASS::MEMBER>

	#define ACCESS(OBJECT, MEMBER) \
	(OBJECT).*Access((T_##MEMBER<std::remove_reference<decltype(OBJECT)>::type>*)nullptr)

} // namespace

namespace log4cxx { namespace classDesc {

	// org.apache.log4j.spi.LoggingEvent
	static const unsigned char LOGGING_EVENT[] = {
		0x72, 0x00, 0x21,
		0x6F, 0x72, 0x67, 0x2E, 0x61, 0x70, 0x61, 0x63,
		0x68, 0x65, 0x2E, 0x6C, 0x6F, 0x67, 0x34, 0x6A,
		0x2E, 0x73, 0x70, 0x69, 0x2E, 0x4C, 0x6F, 0x67,
		0x67, 0x69, 0x6E, 0x67, 0x45, 0x76, 0x65, 0x6E,
		0x74, 0xF3, 0xF2, 0xB9, 0x23, 0x74, 0x0B, 0xB5,
		0x3F, 0x03, 0x00, 0x0A, 0x5A, 0x00, 0x15, 0x6D,
		0x64, 0x63, 0x43, 0x6F, 0x70, 0x79, 0x4C, 0x6F,
		0x6F, 0x6B, 0x75, 0x70, 0x52, 0x65, 0x71, 0x75,
		0x69, 0x72, 0x65, 0x64, 0x5A, 0x00, 0x11, 0x6E,
		0x64, 0x63, 0x4C, 0x6F, 0x6F, 0x6B, 0x75, 0x70,
		0x52, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64,
		0x4A, 0x00, 0x09, 0x74, 0x69, 0x6D, 0x65, 0x53,
		0x74, 0x61, 0x6D, 0x70, 0x4C, 0x00, 0x0C, 0x63,
		0x61, 0x74, 0x65, 0x67, 0x6F, 0x72, 0x79, 0x4E,
		0x61, 0x6D, 0x65, 0x74, 0x00, 0x12, 0x4C, 0x6A,
		0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67,
		0x2F, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3B,
		0x4C, 0x00, 0x0C, 0x6C, 0x6F, 0x63, 0x61, 0x74,
		0x69, 0x6F, 0x6E, 0x49, 0x6E, 0x66, 0x6F, 0x74,
		0x00, 0x23, 0x4C, 0x6F, 0x72, 0x67, 0x2F, 0x61,
		0x70, 0x61, 0x63, 0x68, 0x65, 0x2F, 0x6C, 0x6F,
		0x67, 0x34, 0x6A, 0x2F, 0x73, 0x70, 0x69, 0x2F,
		0x4C, 0x6F, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E,
		0x49, 0x6E, 0x66, 0x6F, 0x3B, 0x4C, 0x00, 0x07,
		0x6D, 0x64, 0x63, 0x43, 0x6F, 0x70, 0x79, 0x74,
		0x00, 0x15, 0x4C, 0x6A, 0x61, 0x76, 0x61, 0x2F,
		0x75, 0x74, 0x69, 0x6C, 0x2F, 0x48, 0x61, 0x73,
		0x68, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x3B, 0x4C,
		0x00, 0x03, 0x6E, 0x64, 0x63,
		0x74, 0x00, 0x12, 0x4C, 0x6A,
		0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67,
		0x2F, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3B,
		0x4C, 0x00, 0x0F, 0x72, 0x65, 0x6E,
		0x64, 0x65, 0x72, 0x65, 0x64, 0x4D, 0x65, 0x73,
		0x73, 0x61, 0x67, 0x65,
		0x74, 0x00, 0x12, 0x4C, 0x6A,
		0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67,
		0x2F, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3B,
		0x4C, 0x00, 0x0A, 0x74, 0x68, 0x72, 0x65,
		0x61, 0x64, 0x4E, 0x61, 0x6D, 0x65,
		0x74, 0x00, 0x12, 0x4C, 0x6A,
		0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67,
		0x2F, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3B,
		0x4C, 0x00, 0x0D, 0x74, 0x68,
		0x72, 0x6F, 0x77, 0x61, 0x62, 0x6C, 0x65, 0x49,
		0x6E, 0x66, 0x6F, 0x74, 0x00, 0x2B, 0x4C, 0x6F,
		0x72, 0x67, 0x2F, 0x61, 0x70, 0x61, 0x63, 0x68,
		0x65, 0x2F, 0x6C, 0x6F, 0x67, 0x34, 0x6A, 0x2F,
		0x73, 0x70, 0x69, 0x2F, 0x54, 0x68, 0x72, 0x6F,
		0x77, 0x61, 0x62, 0x6C, 0x65, 0x49, 0x6E, 0x66,
		0x6F, 0x72, 0x6D, 0x61, 0x74, 0x69, 0x6F, 0x6E,
		0x3B, 0x78, 0x70
	};

	// org.apache.log4j.spi.LocationInfo
	static const unsigned char LOCATION_INFO[] = {
		0x72, 0x00, 0x21, 0x6F, 0x72, 0x67, 0x2E,
		0x61, 0x70, 0x61, 0x63, 0x68, 0x65, 0x2E, 0x6C,
		0x6F, 0x67, 0x34, 0x6A, 0x2E, 0x73, 0x70, 0x69,
		0x2E, 0x4C, 0x6F, 0x63, 0x61, 0x74, 0x69, 0x6F,
		0x6E, 0x49, 0x6E, 0x66, 0x6F, 0xED, 0x99, 0xBB,
		0xE1, 0x4A, 0x91, 0xA5, 0x7C, 0x02, 0x00, 0x01,
		0x4C, 0x00, 0x08, 0x66, 0x75, 0x6C, 0x6C, 0x49,
		0x6E, 0x66, 0x6F,
		0x74, 0x00, 0x12, 0x4C, 0x6A,
		0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67,
		0x2F, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3B,
		0x78, 0x70
	};

	// java.util.Hashtable
	unsigned char HASH_TABLE[] = {
		0x72, 0x00, 0x13, 0x6A, 0x61, 0x76, 0x61,
		0x2E, 0x75, 0x74, 0x69, 0x6C, 0x2E, 0x48, 0x61,
		0x73, 0x68, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x13,
		0xBB, 0x0F, 0x25, 0x21, 0x4A, 0xE4, 0xB8, 0x03,
		0x00, 0x02, 0x46, 0x00, 0x0A, 0x6C, 0x6F, 0x61,
		0x64, 0x46, 0x61, 0x63, 0x74, 0x6F, 0x72, 0x49,
		0x00, 0x09, 0x74, 0x68, 0x72, 0x65, 0x73, 0x68,
		0x6F, 0x6C, 0x64, 0x78, 0x70
	};

}} // log4cxx::classDesc

namespace log4cxx { namespace io  {

	enum { STREAM_MAGIC = 0xACED };
	enum { STREAM_VERSION = 5 };
	enum {
		TC_NULL = 0x70,
		TC_REFERENCE = 0x71,
		TC_CLASSDESC = 0x72,
		TC_OBJECT = 0x73,
		TC_STRING = 0x74,
		TC_ARRAY = 0x75,
		TC_CLASS = 0x76,
		TC_BLOCKDATA = 0x77,
		TC_ENDBLOCKDATA = 0x78
	};
	enum {
		SC_WRITE_METHOD = 0x01,
		SC_SERIALIZABLE = 0x02
	};

}} // log4cxx::io

namespace log4cxx { namespace io {

	std::vector<char> loadFile(const char* filename)
	{
		struct FileCloseDeleter
		{
			inline void operator()(FILE* fp) const
			{
				fclose(fp);
			}
		}; // struct FileCloseDeleter 
		using AutoFilePtr = std::unique_ptr<std::remove_pointer<FILE*>::type, FileCloseDeleter>;

		auto getFileContents = [](const char* filename) -> std::vector<char> {
			FILE* file = fopen(filename, "rb");
			if (!file) {
				fprintf(stderr, "Failed to open: %s\n", filename);
				return std::vector<char>();
			}
			(void)fseek(file, 0, SEEK_END);
			size_t file_length = ftell(file);
			if (!file_length) {
				return std::vector<char>();
			}
			(void)fseek(file, 0, SEEK_SET);

			std::vector<char> buffer(file_length, 0);
			size_t bytes_read = fread(&buffer[0], 1, file_length, file);
			(void)fclose(file);
			if (bytes_read != file_length) {
				fprintf(stderr, "Failed to read: %s\n", filename);
				return std::vector<char>();
			}
			return buffer;
		};

		return getFileContents(filename);
	}

	size_t readStart(const ByteBuf& byteBuf) /*throw(InstantiationException, RuntimeException)*/
	{
		const size_t pos = 0;
		const char* pBuf = &byteBuf[0] + 0;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		// STREAM_MAGIC, STREAM_VERSION
		unsigned char start[] = {
			0xAC, 0xED, 0x00, 0x05
		};
		size_t size = sizeof(start);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		int ret = memcmp(start, pBuf + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			throw helpers::RuntimeException(LOG4CXX_STR("시작 데이터가 잘못 되었다."));
		}
		readSize += size;

		return readSize;
	}

	size_t readProlog(
		const ByteBuf& byteBuf,
		size_t pos,
		const unsigned char* classDesc,
		size_t classDescLen,
		std::pair<std::string, unsigned int>& value
	) /*throw(InstantiationException, RuntimeException)*/ {
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_OBJECT;
		size_t size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		int ret = memcmp(&type, pBuf + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			throw helpers::RuntimeException(LOG4CXX_STR("type이 TC_OBJECT이여야만 한다."));
		}
		readSize += size;

		size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}
		memcpy(&type, pBuf + readSize, size);
		readSize += size;

		switch (type)
		{
		case TC_CLASSDESC:
			{
				size = classDescLen - sizeof(classDesc[0]);
				if (pos + readSize + size > byteBufSize) {
					throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
				}

				ret = memcmp(classDesc + 1, pBuf + readSize, size);
				_ASSERTE(ret == 0 && "memcmp() Failed");
				if (ret != 0) {
					throw helpers::RuntimeException(LOG4CXX_STR("TC_CLASSDESC의 클래스 정보가 잘못되었다."));
				}
				readSize += size;
			}
			break;
		case TC_REFERENCE:
			{
				unsigned char bytes[4] = { 0, };
				size = sizeof(bytes);
				if (pos + readSize + size > byteBufSize) {
					throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
				}

				memcpy(bytes, pBuf + readSize, size);
				readSize += size;

				// 리틀 엔디안으로 변경
				std::swap(bytes[0], bytes[3]);
				std::swap(bytes[1], bytes[2]);
				memcpy(&value.second, bytes, sizeof(bytes));
			}
			break;
		default:
			_ASSERTE(!"type is invalid");
			throw helpers::RuntimeException(LOG4CXX_STR("type 정보를 처리 할수 없다."));
		}

		return readSize;
	}

	size_t readLocationInfo(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(InstantiationException, RuntimeException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_NULL;
		size_t size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}
		memcpy(&type, pBuf + readSize, size);

		if (type == TC_NULL) {
			readSize += size;

			value.clear();
		} else {
			std::pair<std::string, unsigned int> prolog;
			size = readProlog(byteBuf, pos, classDesc::LOCATION_INFO, sizeof(classDesc::LOCATION_INFO), prolog);
			readSize += size;

			std::string fullInfo;
			size = readUTFString(byteBuf, pos + readSize, fullInfo);
			readSize += size;

			value = fullInfo;
		}
		
		return readSize;
	}

	size_t readMDC(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(InstantiationException, RuntimeException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_NULL;
		size_t size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}
		memcpy(&type, pBuf + readSize, size);

		if (type == TC_NULL) {
			readSize += size;

			value.clear();
		} else {
			MDC::Map mdcMap;
			size = readObject(byteBuf, pos, mdcMap);
			readSize += size;

			value = mdcMap;
		}
		
		return readSize;
	}

	size_t readNDC(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(InstantiationException, RuntimeException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_NULL;
		size_t size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}
		memcpy(&type, pBuf + readSize, size);

		if (type == TC_NULL) {
			readSize += size;

			value.clear();
		} else {
			LogString ndc;
			size = readLogString(byteBuf, pos, ndc);
			readSize += size;

			value = ndc;
		}
		
		return readSize;
	}

	size_t readObject(const ByteBuf& byteBuf, size_t pos, MDC::Map& value) /*throw(InstantiationException, RuntimeException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		std::pair<std::string, unsigned int> prolog;
		size_t size = readProlog(byteBuf, pos, classDesc::HASH_TABLE, sizeof(classDesc::HASH_TABLE), prolog);
		readSize += size;

		// == os->write(dataBuf, p);
		const char data[] = {
			0x3F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
			TC_BLOCKDATA, 0x08, 0x00, 0x00, 0x00, 0x07
		};
		size = sizeof(data);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		int ret = memcmp(data, pBuf + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			throw helpers::RuntimeException(LOG4CXX_STR("data 정보가 잘못 되었다."));
		}
		readSize += size;

		// == os->write(sizeBuf, p);
		int mapSize = 0;
		size = readInt(byteBuf, pos + readSize, mapSize);
		readSize += size;

		for (int i = 0; i < mapSize; i++) {
			LogString first, second;
			// <==> writeObject(iter->first, p);
			size = readLogString(byteBuf, pos + readSize, first);
			readSize += size;

			// <==> writeObject(iter->second, p);
			size = readLogString(byteBuf, pos + readSize, second);
			readSize += size;

			value[first] = second;
		}

		// == writeByte(TC_ENDBLOCKDATA, p);
		{
			unsigned char value = TC_ENDBLOCKDATA;
			size = readByte(byteBuf, pos + readSize, value);
			if (value != TC_ENDBLOCKDATA) {
				throw helpers::RuntimeException(LOG4CXX_STR("value는 TC_ENDBLOCKDATA 여야만 한다."));
			}
			readSize += size;
		}

		return readSize;
	}

	size_t readByte(const ByteBuf& byteBuf, size_t pos, unsigned char& value) /*throw(InstantiationException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_NULL;
		size_t size = 1;
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		memcpy(&value, pBuf + readSize, size);
		readSize += size;

		return readSize;
	}

	size_t readBytes(const ByteBuf& byteBuf, size_t pos, size_t bytes, std::vector<char>& value) /*throw(InstantiationException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		size_t size = bytes;
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		value = std::vector<char>(bytes, 0);
		memcpy(&value[0], pBuf + readSize, size);
		readSize += size;

		return readSize;
	}

	size_t readLong(const ByteBuf& byteBuf, size_t pos, log4cxx_time_t& value) /*throw(InstantiationException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		_ASSERTE(sizeof(log4cxx_time_t) == 8 && "읽을 데이터의 크기는 8이여야 한다.");
		const size_t size = 8;
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		// 빅 엔디안
		memcpy(&value, pBuf + readSize, size);

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
		readSize += size;
		return readSize;
	}

	size_t readInt(const ByteBuf& byteBuf, size_t pos, int& value) /*throw(InstantiationException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		_ASSERTE(sizeof(int) == 4 && "읽을 데이터의 크기는 4이여야 한다.");
		const size_t size = 4;
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		// 빅 엔디안
		memcpy(&value, pBuf + readSize, size);

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
		memcpy(bytes, pBuf + readSize, size);

		// 리틀 엔디안으로 변경
		std::swap(bytes[0], bytes[3]);
		std::swap(bytes[1], bytes[2]);

		memcpy(&value, bytes, sizeof(bytes));
*/
		readSize += size;
		return readSize;
	}

	size_t readLogString(const ByteBuf& byteBuf, size_t pos, LogString& value) /*throw(InstantiationException, RuntimeException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_STRING;
		size_t size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		int ret = memcmp(&type, pBuf + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			throw helpers::RuntimeException(LOG4CXX_STR("type이 TC_STRING이여야만 한다."));
		}
		readSize += size;

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		{
			char bytes[2];
			size = sizeof(bytes);
			if (pos + readSize + size > byteBufSize) {
				throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}

			// 빅 엔디안
			memcpy(bytes, pBuf + readSize, size);

			// 리틀 엔디안으로 변경
			std::swap(bytes[0], bytes[1]);
			memcpy(&dataLen, bytes, sizeof(bytes));

			readSize += size;
		}
		
		// UTF 스트링 복사
		std::vector<char> data(dataLen, 0);
		{
			size = data.size();
			if (pos + readSize + size > byteBufSize) {
				throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}
			memcpy(&data[0], pBuf + readSize, size);

			readSize += size;
		}
		
		// UTF-8 스트링 -> LogString으로 변환
		{
			log4cxx::helpers::CharsetDecoderPtr utf8Decoder(log4cxx::helpers::CharsetDecoder::getUTF8Decoder());
			log4cxx::helpers::ByteBuffer buf(&data[0], data.size());
			utf8Decoder->decode(buf, value);
		}
		
		return readSize;
	}

	size_t readUTFString(const ByteBuf& byteBuf, size_t pos, std::string& value) /*throw(InstantiationException, RuntimeException)*/
	{
		const char* pBuf = &byteBuf[0] + pos;
		const size_t byteBufSize = byteBuf.size();
		size_t readSize = 0;

		unsigned char type = TC_STRING;
		size_t size = sizeof(type);
		if (pos + readSize + size > byteBufSize) {
			throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
		}

		int ret = memcmp(&type, pBuf + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			throw helpers::RuntimeException(LOG4CXX_STR("type이 TC_STRING이여야만 한다."));
		}
		readSize += size;

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		{
			char bytes[2];
			size = sizeof(bytes);
			if (pos + readSize + size > byteBufSize) {
				throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}

			// 빅 엔디안
			memcpy(bytes, pBuf + readSize, size);

			// 리틀 엔디안으로 변경
			std::swap(bytes[0], bytes[1]);
			memcpy(&dataLen, bytes, sizeof(bytes));

			readSize += size;
		}
		
		// UTF 스트링 복사
		std::vector<char> data(dataLen, 0);
		{
			size = data.size();
			if (pos + readSize + size > byteBufSize) {
				throw helpers::InstantiationException(LOG4CXX_STR("버퍼 사이즈가 작아 읽을 수가 없다."));
			}
			memcpy(&data[0], pBuf + readSize, size);

			readSize += size;
		}

		// 가장뒤에 0 추가
		data.push_back(0);
		value = &data[0];

		return readSize;
	}

}} // // log4cxx::io

namespace log4cxx { namespace loader {

	auto getClassName = [](const std::string& fullInfo) -> std::string {

		std::string className;
		size_t iend = fullInfo.find_last_of('(');
		if (iend == std::string::npos) {
			className = log4cxx::spi::LocationInfo::NA;
		} else {
			iend = fullInfo.find_last_of('.', iend);
			size_t ibegin = 0;
			if (iend == std::string::npos) {
				className = log4cxx::spi::LocationInfo::NA;
			} else {
				size_t count = iend - ibegin;
				className = fullInfo.substr(ibegin, count);
			}
		}

		return className;
	};

	auto getMethodName = [](const std::string& fullInfo) -> std::string {

		std::string methodName;
		size_t iend = fullInfo.find_last_of('(');
		size_t ibegin = fullInfo.find_last_of('.', iend);
		if (ibegin == std::string::npos) {
			methodName = log4cxx::spi::LocationInfo::NA;
		}
		else {
			size_t count = iend - (ibegin + 1);
			methodName = fullInfo.substr(ibegin + 1, count);
		}

		return methodName;
	};

	auto getFileName = [](const std::string& fullInfo) -> std::string {

		std::string fileName;
		size_t iend = fullInfo.find_last_of(':');
		if (iend == std::string::npos) {
			fileName = log4cxx::spi::LocationInfo::NA;
		}
		else {
			size_t ibegin = fullInfo.find_last_of('(', iend - 1);
			size_t count = iend - (ibegin + 1);
			fileName = fullInfo.substr(ibegin + 1, count);
		}

		return fileName;
	};

	auto getLineNumber = [](const std::string& fullInfo) -> std::string {

		std::string lineNumber;
		size_t iend = fullInfo.find_last_of(')');
		size_t ibegin = fullInfo.find_last_of(':', iend - 1);
		if (ibegin == std::string::npos) {
			lineNumber = log4cxx::spi::LocationInfo::NA;
		}
		else {
			size_t count = iend - (ibegin + 1);
			lineNumber = fullInfo.substr(ibegin + 1, count);
		}

		return lineNumber;
	};

	class LoggingEventEx :
		public virtual spi::LoggingEvent
	{
	public:
		LoggingEventEx(const LogString& logger,
			const LevelPtr& level, const LogString& message,
			const std::string& fullInfo
		) : spi::LoggingEvent(logger, level, message, spi::LocationInfo())
		{
			m_FileNameFromStream = getFileName(fullInfo);
			m_MethodNameFromStream = getMethodName(fullInfo);
			std::string lineNumber = getLineNumber(fullInfo);

			const_cast<spi::LocationInfo&>(getLocationInformation()) = spi::LocationInfo(
				m_FileNameFromStream.c_str(),
				m_MethodNameFromStream.c_str(),
				std::atoi(lineNumber.c_str())
			);
		}
		~LoggingEventEx()
		{
		}

	private:
		std::string m_FileNameFromStream;
		std::string m_MethodNameFromStream;
	}; // class LoggingEventEx
	
	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, timeStamp, log4cxx_time_t);
	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, threadName, const LogString);
	log4cxx::spi::LoggingEventPtr createLoggingEvent(const ByteBuf& byteBuf, size_t& readBytes) /*throw(InstantiationException, RuntimeException, bad_alloc)*/
	{
		const char* pBuf = &byteBuf[0];
		size_t pos = 0;

		// <==> writeProlog(os, p);
		{
			std::pair<std::string, unsigned int> value;
			size_t size = io::readProlog(byteBuf, pos, classDesc::LOGGING_EVENT, sizeof(classDesc::LOGGING_EVENT), value);
			pos += size;
		}

		// <==> os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			std::vector<char> value;
			size_t size = io::readBytes(byteBuf, pos, sizeof(lookupsRequired), value);
			int ret = memcmp(lookupsRequired, &value[0], sizeof(lookupsRequired));
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				throw helpers::RuntimeException(LOG4CXX_STR("lookupsRequired와 value는 같아야 한다."));
			}
			pos += size;
		}

		// <==> os.writeLong(timeStamp/1000, p);
		log4cxx_time_t timeStamp = 0;
		{
			size_t size = io::readLong(byteBuf, pos, timeStamp);
			timeStamp *= 1000;
			pos += size;
		}

		// <==> os.writeObject(logger, p);
		LogString logger;
		{
			size_t size = io::readLogString(byteBuf, pos, logger);
			pos += size;
		}

		// <==> locationInfo.write(os, p);
		std::string fullInfo;
		{
			size_t size = io::readLocationInfo(byteBuf, pos, fullInfo);
			pos += size;
		}

		// <==> mdc
		MDC::Map mdcMap;
		{
			size_t size = io::readMDC(byteBuf, pos, mdcMap);
			pos += size;
		}

		// <==> ndc 
		LogString ndc;
		{
			size_t size = io::readNDC(byteBuf, pos, ndc);
			pos += size;
		}

		// <==> os.writeObject(message, p);
		LogString message;
		{
			size_t size = io::readLogString(byteBuf, pos, message);
			pos += size;
		}

		// <==> os.writeObject(threadName, p);
		LogString threadName;
		{
			size_t size = io::readLogString(byteBuf, pos, threadName);
			pos += size;
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = io::TC_NULL;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != io::TC_NULL) {
				throw helpers::RuntimeException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			unsigned char value = io::TC_BLOCKDATA;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != io::TC_BLOCKDATA) {
				throw helpers::RuntimeException(LOG4CXX_STR("value는 TC_BLOCKDATA 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(0x04, p);
		{
			unsigned char value = 0x04;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != 0x04) {
				throw helpers::RuntimeException(LOG4CXX_STR("value는 0x04 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeInt(level->toInt(), p);
		int level = 0;
		{
			size_t size = io::readInt(byteBuf, pos, level);
			pos += size;
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = io::TC_NULL;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != io::TC_NULL) {
				throw helpers::RuntimeException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			unsigned char value = io::TC_ENDBLOCKDATA;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != io::TC_ENDBLOCKDATA) {
				throw helpers::RuntimeException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		_ASSERTE((byteBuf.size() >= pos) && "모든 값을 읽지 못했다.");
		readBytes = pos;

		log4cxx::spi::LoggingEventPtr event(
			new LoggingEventEx(
				logger,
				Level::toLevel(level),
				message,
				fullInfo
			)
		);
		// backdoor
		ACCESS(*event, timeStamp) = timeStamp;
		const_cast<LogString&>(ACCESS(*event, threadName)) = threadName;
		// ==
		// const_cast<LogString&>(event->getThreadName()) = threadName;

		return event;
	}

}} // log4cxx::factory
