// ObjectLoader.cpp
#ifdef _WIN32
#	include <winsock2.h> // SOCKET
#	include <crtdbg.h> // _ASSERTE
#else
typedef int SOCKET;
#	include <assert.h>
#	define _ASSERTE assert
#endif

#include "ObjectLoader.h"
#include "InputStreamDef.h"
#include "ByteBufInputStream.h"
#include "SocketInputStream.h"

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

namespace log4cxx { namespace ext { namespace io {

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

	bool readStart(int socket) noexcept
	{
		// STREAM_MAGIC, STREAM_VERSION
		unsigned char start[] = {
			0xAC, 0xED, 0x00, 0x05
		};
		size_t size = sizeof(start);
		std::vector<unsigned char> byteBuf;
		if (!io::readBytes(socket, size, byteBuf)) {
			return false;
		}
		int ret = memcmp(start, &byteBuf[0], size);
		_ASSERTE(ret == 0 && "memcmp() Failed");
		if (ret != 0) {
			return false;
		}

		return true;
	}

	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, timeStamp, log4cxx_time_t);
	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, threadName, const LogString);
	log4cxx::spi::LoggingEventPtr createLoggingEvent(ByteBuf& byteBuf) /*throw(SmallBufferException, InvalidBufferException)*/
	{
		const char* pBuf = &byteBuf[0];
		size_t pos = 0;

		// <==> writeProlog(os, p);
		{
			unsigned char typeClass = io::TC_OBJECT;
			size_t size = io::readByte(byteBuf, pos, typeClass);
			if (typeClass != io::TC_OBJECT) {
				throw InvalidBufferException(LOG4CXX_STR("type이 TC_OBJECT이여야만 한다."));
			}
			pos += size;

			std::pair<std::string, unsigned int> value;
			size = io::readProlog(byteBuf, pos, classDesc::LOGGING_EVENT, sizeof(classDesc::LOGGING_EVENT), value);
			pos += size;
		}

		// <==> os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			std::vector<unsigned char> value;
			size_t size = io::readBytes(byteBuf, pos, sizeof(lookupsRequired), value);
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
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			unsigned char value = io::TC_BLOCKDATA;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != io::TC_BLOCKDATA) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_BLOCKDATA 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(0x04, p);
		{
			unsigned char value = 0x04;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != 0x04) {
				throw InvalidBufferException(LOG4CXX_STR("value는 0x04 여야만 한다."));
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
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		// <==> os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			unsigned char value = io::TC_ENDBLOCKDATA;
			size_t size = io::readByte(byteBuf, pos, value);
			if (value != io::TC_ENDBLOCKDATA) {
				throw InvalidBufferException(LOG4CXX_STR("value는 TC_NULL 여야만 한다."));
			}
			pos += size;
		}

		_ASSERTE((byteBuf.size() >= pos) && "모든 값을 읽지 못했다.");
		size_t readBytes = pos;
		byteBuf.erase(byteBuf.begin(), byteBuf.begin() + readBytes);

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

	log4cxx::spi::LoggingEventPtr createLoggingEvent(SOCKET socket) noexcept
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
