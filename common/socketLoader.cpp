// socketLoader.cpp
#include "socketLoader.h"
#include <log4cxx/helpers/exception.h> // SmallBufferException, InvalidBufferException
#include <log4cxx/helpers/charsetdecoder.h> // log4cxx::helpers::CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer
#include <memory> // std::unique_ptr
#include <memory.h> // memcmp

#ifdef _WIN32
#	include <crtdbg.h> // _ASSERTE
#	include <winsock2.h> // recv
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

namespace log4cxx { namespace ext { namespace classDesc {

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
	static unsigned char HASH_TABLE[] = {
		0x72, 0x00, 0x13, 0x6A, 0x61, 0x76, 0x61,
		0x2E, 0x75, 0x74, 0x69, 0x6C, 0x2E, 0x48, 0x61,
		0x73, 0x68, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x13,
		0xBB, 0x0F, 0x25, 0x21, 0x4A, 0xE4, 0xB8, 0x03,
		0x00, 0x02, 0x46, 0x00, 0x0A, 0x6C, 0x6F, 0x61,
		0x64, 0x46, 0x61, 0x63, 0x74, 0x6F, 0x72, 0x49,
		0x00, 0x09, 0x74, 0x68, 0x72, 0x65, 0x73, 0x68,
		0x6F, 0x6C, 0x64, 0x78, 0x70
	};

}}} // log4cxx::ext::classDesc

namespace log4cxx { namespace ext { namespace io  {

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

}}} // log4cxx::ext::io

namespace log4cxx { namespace ext { namespace io {

	bool read(int socket, void* buf, size_t len) noexcept
	{
		int len_read = 0;
		unsigned char* p = (unsigned char*)buf;

		while ((size_t)(p - (unsigned char*)buf) < len) {
#ifdef _WIN32
			len_read = ::recv(socket, (char*)p, static_cast<int>(len - (p - (unsigned char*)buf)), 0);
#else
			len_read = ::read(socket, p, len - (p - (unsigned char*)buf));
#endif
			if (len_read < 0) {
				return false;
			}
			if (len_read == 0) {
				return false;
			}

			p += len_read;
		}

		bool result = (len == (p - (const unsigned char*)buf)) ? true : false;
		_ASSERTE(result && "len 사이즈 만큼 읽어야만 한다.");

		return result;
	}

	bool readByte(int socket, unsigned char& value) noexcept
	{
		return read(socket, &value, sizeof(unsigned char));
	}

	bool readBytes(int socket, size_t bytes, std::vector<unsigned char>& value) noexcept
	{
		value = std::vector<unsigned char>(bytes, 0);
		if (!read(socket, &value[0], bytes)) {
			return false;
		}

		return true;
	}

	bool readInt(int socket, int& value) noexcept
	{
		_ASSERTE(sizeof(int) == 4 && "읽을 데이터의 크기는 4이여야 한다.");
		if (!read(socket, &value, sizeof(int))) {
			return false;
		}

		// 리틀 엔디안으로 변경
		char bytes[4] = { 0, };
		bytes[3] = (char)(value & 0xFF);
		bytes[2] = (char)((value >> 8) & 0xFF);
		bytes[1] = (char)((value >> 16) & 0xFF);
		bytes[0] = (char)((value >> 24) & 0xFF);

		memcpy(&value, bytes, sizeof(bytes));

		return true;
	}

	bool readLong(int socket, log4cxx_time_t& value) noexcept
	{
		_ASSERTE(sizeof(log4cxx_time_t) == 8 && "읽을 데이터의 크기는 8이여야 한다.");
		if (!read(socket, &value, sizeof(log4cxx_time_t))) {
			return false;
		}

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

		return true;
	}

	bool readUTFString(int socket, std::string& value, bool skipTypeClass /*= false*/) noexcept
	{
		if (!skipTypeClass) {
			unsigned char typeClass = TC_STRING;
			if (!readByte(socket, typeClass) || typeClass != TC_STRING) {
				return false;
			}
		}

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		char bytes[2];
		if (!read(socket, bytes, sizeof(bytes))) {
			return false;
		}

		// 리틀 엔디안으로 변경
		std::swap(bytes[0], bytes[1]);
		memcpy(&dataLen, bytes, sizeof(bytes));

		// UTF 스트링 복사
		std::vector<char> data(dataLen, 0);
		if (!read(socket, &data[0], dataLen)) {
			return false;
		}

		// 가장뒤에 0 추가
		data.push_back(0);
		value = &data[0];

		return true;
	}

	bool readLogString(int socket, LogString& value, bool skipTypeClass /*= false*/) noexcept
	{
		if (!skipTypeClass) {
			unsigned char typeClass = TC_STRING;
			if (!readByte(socket, typeClass) || typeClass != TC_STRING) {
				return false;
			}
		}

		// UTF 스트링 길이 구함
		size_t dataLen = 0;
		char bytes[2];
		if (!read(socket, bytes, sizeof(bytes))) {
			return false;
		}

		// 리틀 엔디안으로 변경
		std::swap(bytes[0], bytes[1]);
		memcpy(&dataLen, bytes, sizeof(bytes));

		// UTF 스트링 복사
		std::vector<char> data(dataLen, 0);
		if (!read(socket, &data[0], dataLen)) {
			return false;
		}

		// UTF-8 스트링 -> LogString으로 변환
		log4cxx::helpers::CharsetDecoderPtr utf8Decoder(log4cxx::helpers::CharsetDecoder::getUTF8Decoder());
		log4cxx::helpers::ByteBuffer buf(const_cast<char*>(&data[0]), data.size());
		utf8Decoder->decode(buf, value);

		return true;
	}

	bool readObject(int socket, MDC::Map& value, bool skipTypeClass /*= false*/) noexcept
	{
		if (!skipTypeClass) {
			unsigned char typeClass = TC_OBJECT;
			if (!readByte(socket, typeClass) || typeClass != TC_OBJECT) {
				return false;
			}
		}

		std::pair<std::string, unsigned int> prolog;
		if (!readProlog(socket, classDesc::HASH_TABLE, sizeof(classDesc::HASH_TABLE), prolog)) {
			return false;
		}

		// == os->write(dataBuf, p);
		{
			const char data[] = {
				0x3F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
				TC_BLOCKDATA, 0x08, 0x00, 0x00, 0x00, 0x07
			};
			size_t size = sizeof(data);
			std::vector<unsigned char> byteBuf;
			if (!readBytes(socket, size, byteBuf)) {
				return false;
			}
			int ret = memcmp(data, &byteBuf[0], size);
			_ASSERTE(ret == 0 && "memcmp() Failed");
			if (ret != 0) {
				return false;
			}
		}

		// == os->write(sizeBuf, p);
		{
			int mapSize = 0;
			if (!readInt(socket, mapSize)) {
				return false;
			}

			for (int i = 0; i < mapSize; i++) {
				LogString first, second;

				// <==> writeObject(iter->first, p);
				if (!readLogString(socket, first)) {
					return false;
				}

				// <==> writeObject(iter->second, p);
				if (!readLogString(socket, second)) {
					return false;
				}

				value[first] = second;
			}
		}

		// == writeByte(TC_ENDBLOCKDATA, p);
		{
			unsigned char type = TC_ENDBLOCKDATA;
			if (!readByte(socket, type) || type != TC_ENDBLOCKDATA) {
				return false;
			}
		}

		return true;
	}

	bool readProlog(
		int socket,
		const unsigned char* classDesc,
		size_t classDescLen,
		std::pair<std::string, unsigned int>& value
	) noexcept {
		unsigned char typeClass = TC_CLASSDESC;
		if (!readByte(socket, typeClass)) {
			return false;
		}

		switch (typeClass)
		{
		case TC_CLASSDESC:
			{
				size_t size = classDescLen - sizeof(classDesc[0]);
				std::vector<unsigned char> byteBuf;
				if (!readBytes(socket, size , byteBuf)) {
					return false;
				}
				int ret = memcmp(classDesc + 1, &byteBuf[0], size);
				_ASSERTE(ret == 0 && "memcmp() Failed");
				if (ret != 0) {
					return false;
				}
			}
			break;
		case TC_REFERENCE:
			{
				int val = 0;
				if (!readInt(socket, val)) {
					return false;
				}

				value.second = val;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			return false;
		}

		return true;
	}

	bool readLocationInfo(int socket, std::string& value) noexcept
	{
		unsigned char typeClass = TC_NULL;
		if (!readByte(socket, typeClass)) {
			return false;
		}

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
				if (!readProlog(socket, classDesc::LOCATION_INFO, sizeof(classDesc::LOCATION_INFO), prolog)) {
					return false;
				}

				std::string fullInfo;
				if (!readUTFString(socket, fullInfo)) {
					return false;
				}

				value = fullInfo;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			return false;
		}
	}
	
	bool readMDC(int socket, MDC::Map& value) noexcept
	{
		unsigned char typeClass = TC_NULL;
		if (!readByte(socket, typeClass)) {
			return false;
		}

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
				if (!readObject(socket, mdcMap, true)) {
					return false;
				}

				value = mdcMap;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			return false;
		}

		return true;
	}

	bool readNDC(int socket, LogString& value) noexcept
	{
		unsigned char typeClass = TC_NULL;
		if (!readByte(socket, typeClass)) {
			return false;
		}

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
				if (!readLogString(socket, ndc, true)) {
					return false;
				}

				value = ndc;
			}
			break;
		default:
			_ASSERTE(!"typeClass is invalid");
			return false;
		}

		return true;
	}

	bool readStart(int socket) noexcept
	{
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

}}} // // log4cxx::ext::io

namespace log4cxx { namespace ext { namespace loader {

	static auto getClassName = [](const std::string& fullInfo) -> std::string {

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

	static auto getMethodName = [](const std::string& fullInfo) -> std::string {

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

	static auto getFileName = [](const std::string& fullInfo) -> std::string {

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

	static auto getLineNumber = [](const std::string& fullInfo) -> std::string {

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
	log4cxx::spi::LoggingEventPtr createLoggingEvent(int socket) noexcept
	{
		// <==> writeProlog(os, p);
		{
			unsigned char typeClass = io::TC_OBJECT;
			if (!io::readByte(socket, typeClass) || typeClass != io::TC_OBJECT) {
				return nullptr;
			}

			std::pair<std::string, unsigned int> value;
			if (!io::readProlog(socket, classDesc::LOGGING_EVENT, sizeof(classDesc::LOGGING_EVENT), value)) {
				return nullptr;
			}
		}

		// <==> os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			std::vector<unsigned char> value;
			if (!io::readBytes(socket, sizeof(lookupsRequired), value)) {
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
			if (!io::readLong(socket, timeStamp)) {
				return nullptr;
			}
			timeStamp *= 1000;
		}

		// <==> os.writeObject(logger, p);
		LogString logger;
		{
			if (!io::readLogString(socket, logger)) {
				return nullptr;
			}
		}

		// <==> locationInfo.write(os, p);
		std::string fullInfo;
		{
			if (!io::readLocationInfo(socket, fullInfo)) {
				return nullptr;
			}
		}

		// <==> mdc
		MDC::Map mdcMap;
		{
			if (!io::readMDC(socket, mdcMap)) {
				return nullptr;
			}
		}

		// <==> ndc 
		LogString ndc;
		{
			if (!io::readNDC(socket, ndc)) {
				return nullptr;
			}
		}

		// <==> os.writeObject(message, p);
		LogString message;
		{
			if (!io::readLogString(socket, message)) {
				return nullptr;
			}
		}

		// <==> os.writeObject(threadName, p);
		LogString threadName;
		{
			if (!io::readLogString(socket, threadName)) {
				return nullptr;
			}
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = io::TC_NULL;
			if (!io::readByte(socket, value) || value != io::TC_NULL) {
				return nullptr;
			}
		}

		// <==> os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			unsigned char value = io::TC_BLOCKDATA;
			if (!io::readByte(socket, value) || value != io::TC_BLOCKDATA) {
				return nullptr;
			}
		}

		// <==> os.writeByte(0x04, p);
		{
			unsigned char value = 0x04;
			if (!io::readByte(socket, value) || value != 0x04) {
				return nullptr;
			}
		}

		// <==> os.writeInt(level->toInt(), p);
		int level = 0;
		{
			if (!io::readInt(socket, level)) {
				return nullptr;
			}
		}

		// <==> os.writeNull(p);
		{
			unsigned char value = io::TC_NULL;
			if (!io::readByte(socket, value) || value != io::TC_NULL) {
				return nullptr;
			}
		}

		// <==> os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			unsigned char value = io::TC_ENDBLOCKDATA;
			if (!io::readByte(socket, value) || value != io::TC_ENDBLOCKDATA) {
				return nullptr;
			}
		}

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

}}} // log4cxx::ext::loader
