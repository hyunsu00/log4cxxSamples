#include "Packet.h"
#include <memory> // std::unique_ptr
#include <crtdbg.h> // _ASSERTE
#include <log4cxx/helpers/charsetdecoder.h> // CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // ByteBuffer

#define CONCATE_(X, Y) X##Y
#define CONCATE(X, Y) CONCATE_(X, Y)

#define ALLOW_ACCESS(CLASS, MEMBER, ...) \
  template<typename Only, __VA_ARGS__ CLASS::*Member> \
  struct CONCATE(MEMBER, __LINE__) { friend __VA_ARGS__ CLASS::*Access(Only*) { return Member; } }; \
  template<typename> struct Only_##MEMBER; \
  template<> struct Only_##MEMBER<CLASS> { friend __VA_ARGS__ CLASS::*Access(Only_##MEMBER<CLASS>*); }; \
  template struct CONCATE(MEMBER, __LINE__)<Only_##MEMBER<CLASS>, &CLASS::MEMBER>

#define ACCESS(OBJECT, MEMBER) \
(OBJECT).*Access((Only_##MEMBER<std::remove_reference<decltype(OBJECT)>::type>*)nullptr)

namespace log4cxx { namespace helpers {

	std::vector<char> loadPacket(const char* filename)
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

	bool beginPacket(const std::vector<char>& packet)
	{
		unsigned char start[] = {
			0xAC, 0xED, 0x00, 0x05
		};
		size_t size = sizeof(start);
		int ret = memcmp(start, &packet[0], size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return false;
		}

		return true;
	}

	bool parsePacket(const std::vector<char>& packet)
	{
		const char* pBuf = &packet[0];
		size_t pos = 0;
		// == writeProlog(os, p);
		{
			unsigned char classDesc[] = {
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

			//
			unsigned char type = TC_OBJECT;
			size_t size = sizeof(type);
			int ret = memcmp(&type, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;

			type = *(pBuf + pos);
			size = sizeof(type);
			pos += size;
			switch (type)
			{
			case TC_CLASSDESC:
				{
					size = sizeof(classDesc) - sizeof(classDesc[0]);
					ret = memcmp(classDesc + 1, pBuf + pos, size);
					_ASSERTE(ret == 0 && "memcmp() Failed");
					if (ret != 0) {
						return false;
					}
					pos += size;
				}
				break;
			case TC_REFERENCE:
				{
					// 
					unsigned char bytes[4] = { 0, };
					size = sizeof(bytes);
					memcpy(bytes, pBuf + pos, size);
					pos += size;
				}
				break;
			default:
				_ASSERTE(!"type is invalid");
				return false;
			}
		}

		// == os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			size_t size = sizeof(lookupsRequired);
			int ret = memcmp(lookupsRequired, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;
		}

		// == os.writeLong(timeStamp/1000, p);
		{
			log4cxx_time_t timeStamp = 0;
			size_t size = readLong(packet, pos, timeStamp);
			timeStamp *= 1000;
			pos += size;
		}

		// == os.writeObject(logger, p);
		{
			LogString logger;
			size_t size = readLogString(packet, pos, logger);
			pos += size;
		}

		// locationInfo.write(os, p);
		{
			unsigned char type = *(pBuf + pos);
			switch (type)
			{
			// == os.writeNull(p);
			case TC_NULL:
				{
					size_t size = sizeof(type);
					pos += size;
				}
				break;
			// == os.writeProlog("org.apache.log4j.spi.LocationInfo", 2, prolog, sizeof(prolog), p);
			default:
				{
					unsigned char classDesc[] = {
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

					type = TC_OBJECT;
					size_t size = sizeof(type);
					int ret = memcmp(&type, pBuf + pos, size);
					_ASSERTE(ret == 0 && "memcmp() Failed");
					if (ret != 0) {
						return false;
					}
					pos += size;

					type = *(pBuf + pos);
					size = sizeof(type);
					pos += size;
					switch (type)
					{
					case TC_CLASSDESC:
						{
							size = sizeof(classDesc) - sizeof(classDesc[0]);
							ret = memcmp(classDesc + 1, pBuf + pos, size);
							_ASSERTE(ret == 0 && "memcmp() Failed");
							if (ret != 0) {
								return false;
							}
							pos += size;
						}
						break;
					case TC_REFERENCE:
						{
							// 
							unsigned char bytes[4] = { 0, };
							size = sizeof(bytes);
							memcpy(bytes, pBuf + pos, size);
							pos += size;
						}
						break;
					default:
						_ASSERTE(!"type is invalid");
						return false;
					}

					std::string fullInfo;
					size = readUTFString(packet, pos, fullInfo);
					pos += size;
				}
				break;
			}
		}

		// mdc
		{
			unsigned char type = *(pBuf + pos);
			switch (type)
			{
			// == os.writeNull(p);
			case TC_NULL:
				{
					size_t size = sizeof(type);
					pos += size;
				}
				break;
			// == os.writeObject(*mdcCopy, p);
			default:
				{
					MDC::Map mdcMap;
					size_t size = readObject(packet, pos, mdcMap);
					pos += size;
				}
				break;
			}
		}

		// ndc 
		{
			unsigned char type = *(pBuf + pos);
			switch (type)
			{
			// == os.writeNull(p);
			case TC_NULL:
				{
					size_t size = sizeof(type);
					pos += size;
				}
				break;
			// == os.writeObject(*ndc, p);
			default:
				{
					MDC::Map ndcMap;
					size_t size = readObject(packet, pos, ndcMap);
					pos += size;
				}
				break;
			}
		}

		// == os.writeObject(message, p);
		{
			LogString message;
			size_t size = readLogString(packet, pos, message);
			pos += size;
		}
		
		// == os.writeObject(threadName, p);
		{
			LogString threadName;
			size_t size = readLogString(packet, pos, threadName);
			pos += size;
		}

		// == os.writeNull(p);
		{
			unsigned char type = TC_NULL;
			size_t size = sizeof(type);
			int ret = memcmp(&type, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;
		}

		// == os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			unsigned char type = TC_BLOCKDATA;
			size_t size = sizeof(type);
			int ret = memcmp(&type, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;
		}

		// == os.writeByte(0x04, p);
		{
			unsigned char type = 0x04;
			size_t size = sizeof(type);
			int ret = memcmp(&type, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;
		}

		// == os.writeInt(level->toInt(), p);
		{
			int level = 0;
			size_t size = readInt(packet, pos, level);
			pos += size;
		}

		// == os.writeNull(p);
		{
			unsigned char type = TC_NULL;
			size_t size = sizeof(type);
			int ret = memcmp(&type, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;
		}

		// == os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			unsigned char type = TC_ENDBLOCKDATA;
			size_t size = sizeof(type);
			int ret = memcmp(&type, pBuf + pos, size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
			pos += size;
		}

		_ASSERTE((packet.size() == pos) && "모든 값을 읽지 못했다.");

		return true;
	}

	size_t readLong(const std::vector<char>& packet, size_t pos, log4cxx_time_t& value)
	{
		memcpy(&value, &packet[0] + pos, sizeof(log4cxx_time_t));

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
/*
		char bytes[8];
		memcpy(bytes, &packet[0] + pos, 8);

		std::swap(bytes[0], bytes[7]);
		std::swap(bytes[1], bytes[6]);
		std::swap(bytes[2], bytes[5]);
		std::swap(bytes[3], bytes[4]);

		memcpy(&value, bytes, sizeof(bytes));
*/
		return sizeof(log4cxx_time_t);
	}

	size_t readInt(const std::vector<char>& packet, size_t pos, int& value)
	{
		memcpy(&value, &packet[0] + pos, sizeof(int));

		char bytes[4] = { 0, };
		bytes[3] = (char)(value & 0xFF);
		bytes[2] = (char)((value >> 8) & 0xFF);
		bytes[1] = (char)((value >> 16) & 0xFF);
		bytes[0] = (char)((value >> 24) & 0xFF);

		memcpy(&value, bytes, sizeof(bytes));

/*
		char bytes[4];
		memcpy(bytes, &packet[0] + pos, 4);

		std::swap(bytes[0], bytes[3]);
		std::swap(bytes[1], bytes[2]);

		memcpy(&value, bytes, sizeof(bytes));
*/
		return sizeof(int);
	}

	size_t readLogString(const std::vector<char>& packet, size_t pos, LogString& value)
	{
		size_t readSize = 0;

		const char* pBuf = &packet[0];
		unsigned char type = TC_STRING;
		size_t size = sizeof(type);
		int ret = memcmp(&type, pBuf + pos + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return readSize;
		}
		readSize += size;

		//
		size_t dataLen = 0;
		char bytes[2];
		size = sizeof(bytes);
		memcpy(bytes, pBuf + pos + readSize, size);
		std::swap(bytes[0], bytes[1]);
		memcpy(&dataLen, bytes, sizeof(bytes));
		readSize += size;
		
		//
		std::vector<char> data(dataLen, 0);
		size = data.size();
		memcpy(&data[0], pBuf + pos + readSize, size);
		readSize += size;

		//
		CharsetDecoderPtr utf8Decoder(CharsetDecoder::getUTF8Decoder());
		ByteBuffer buf(&data[0], size);
		utf8Decoder->decode(buf, value);

		return readSize;
	}

	size_t readUTFString(const std::vector<char>& packet, size_t pos, std::string& value)
	{
		size_t readSize = 0;

		const char* pBuf = &packet[0];
		unsigned char type = TC_STRING;
		size_t size = sizeof(type);
		int ret = memcmp(&type, pBuf + pos + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return readSize;
		}
		readSize += size;

		//
		size_t dataLen = 0;
		char bytes[2];
		size = sizeof(bytes);
		memcpy(bytes, pBuf + pos + readSize, size);
		std::swap(bytes[0], bytes[1]);
		memcpy(&dataLen, bytes, sizeof(bytes));
		readSize += size;

		//
		std::vector<char> data(dataLen, 0);
		size = data.size();
		memcpy(&data[0], pBuf + pos + readSize, size);
		readSize += size;
		
		//
		data.push_back(0);
		value = &data[0];

		return readSize;
	}

	size_t readObject(const std::vector<char>& packet, size_t pos, MDC::Map& value)
	{
		size_t readSize = 0;
		const char* pBuf = &packet[0];

		unsigned char classDesc[] = {
			0x72, 0x00, 0x13, 0x6A, 0x61, 0x76, 0x61,
			0x2E, 0x75, 0x74, 0x69, 0x6C, 0x2E, 0x48, 0x61,
			0x73, 0x68, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x13,
			0xBB, 0x0F, 0x25, 0x21, 0x4A, 0xE4, 0xB8, 0x03,
			0x00, 0x02, 0x46, 0x00, 0x0A, 0x6C, 0x6F, 0x61,
			0x64, 0x46, 0x61, 0x63, 0x74, 0x6F, 0x72, 0x49,
			0x00, 0x09, 0x74, 0x68, 0x72, 0x65, 0x73, 0x68,
			0x6F, 0x6C, 0x64, 0x78, 0x70 
		};

		// == writeProlog("java.util.Hashtable", 1, prolog, sizeof(prolog), p);
		unsigned char type = TC_OBJECT;
		size_t size = sizeof(type);
		int ret = memcmp(&type, pBuf + pos + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return readSize;
		}
		readSize += size;

		type = *(pBuf + pos + readSize);
		size = sizeof(type);
		readSize += size;
		switch (type)
		{
		case TC_CLASSDESC:
			{
				size = sizeof(classDesc) - sizeof(classDesc[0]);
				ret = memcmp(classDesc + 1, pBuf + pos + readSize, size);
				_ASSERTE(ret == 0 && "memcmp() Failed");
				if (ret != 0) {
					return 0;
				}
				readSize += size;
			}
			break;
		case TC_REFERENCE:
			{
				// 
				unsigned char bytes[4] = { 0, };
				size = sizeof(bytes);
				memcpy(bytes, pBuf + pos + readSize, size);
				readSize += size;
			}
			break;
		default:
			_ASSERTE(!"type is invalid");
			return 0;
		}

		// == os->write(dataBuf, p);
		char data[] = { 
			0x3F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
			TC_BLOCKDATA, 0x08, 0x00, 0x00, 0x00, 0x07 
		};
		size = sizeof(data);
		ret = memcmp(data, pBuf + pos + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return 0;
		}
		readSize += size;

		// == os->write(sizeBuf, p);
		int mapSize = 0;
		size = readInt(packet, pos + readSize, mapSize);
		readSize += size;

		for (int i = 0; i < mapSize; i++) {
			LogString first, second;
			// == writeObject(iter->first, p);
			size = readLogString(packet, pos + readSize, first);
			readSize += size;
			// == writeObject(iter->second, p);
			size = readLogString(packet, pos + readSize, second);
			readSize += size;

			value[first] = second;
		}

		// == writeByte(TC_ENDBLOCKDATA, p);
		type = TC_ENDBLOCKDATA;
		size = sizeof(type);
		ret = memcmp(&type, pBuf + pos + readSize, size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return 0;
		}
		readSize += size;

		return readSize;
	}

	
	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, timeStamp, log4cxx_time_t);
	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, logger, LogString);

	log4cxx::spi::LoggingEventPtr createLoggingEvent(const std::vector<char>& packet)
	{
//		log4cxx::spi::LoggingEventPtr event(new log4cxx::spi::LoggingEvent(name, level1, msg, location));
		log4cxx::spi::LoggingEventPtr event(new log4cxx::spi::LoggingEvent());

		ACCESS(*event, timeStamp) = 42;
		ACCESS(*event, logger) = L"123";

		//const void* pThis = p->cast(log4cxx::spi::LoggingEvent::getStaticClass());
		//log4cxx::spi::LoggingEvent* p2 = (log4cxx::spi::LoggingEvent*)pThis;
		//p2->logger = "root1";


		return event;
	}

}} // namespace log4cxx::helpers
