// LoggingEventEx.cpp
//
#include "LoggingEvnetEx.h"

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
/*
1.
ALLOW_ACCESS(log4cxx::spi::LoggingEvent, timeStamp, log4cxx_time_t);
// ==
using timeStampPtr = log4cxx_time_t log4cxx::spi::LoggingEvent::*;
timeStampPtr _timeStampFromLoggingEvent();
template<timeStampPtr timeStamp>
struct _LoggingEvent {
	friend timeStampPtr _timeStampFromLoggingEvent() {
		return timeStamp;
	}
};
template struct _LoggingEvent<&log4cxx::spi::LoggingEvent::timeStamp>;
2.
ACCESS(*event, timeStamp) = timeStamp;
// ==
(*event).*_timeStampFromLoggingEvent() = timeStamp;
*/

namespace log4cxx { namespace ext { namespace loader {
	
	static auto getClassName = [](const std::string& fullInfo) -> std::string {

		std::string className;
		size_t iend = fullInfo.find_last_of('(');
		if (iend == std::string::npos) {
			className = log4cxx::spi::LocationInfo::NA;
		}
		else {
			iend = fullInfo.find_last_of('.', iend);
			size_t ibegin = 0;
			if (iend == std::string::npos) {
				className = log4cxx::spi::LocationInfo::NA;
			}
			else {
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

	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, timeStamp, log4cxx_time_t);
	ALLOW_ACCESS(log4cxx::spi::LoggingEvent, threadName, const LogString);

	class LoggingEventEx :
		public virtual spi::LoggingEvent
	{
	public:
		LoggingEventEx(const LogString& logger,
			const LevelPtr& level, const LogString& message,
			log4cxx_time_t timeStamp, const LogString& threadName,
			const std::string& filePathName, const std::string& methodName, const std::string& lineNumber
		) : spi::LoggingEvent(logger, level, message, spi::LocationInfo())
		{
			// backdoor
			ACCESS((spi::LoggingEvent&)*this, timeStamp) = timeStamp;
			const_cast<LogString&>(ACCESS((spi::LoggingEvent&)*this, threadName)) = threadName;
			// ==
			// const_cast<LogString&>(event->getThreadName()) = threadName;

			m_FilePathNameFromStream = filePathName;
			m_MethodNameFromStream = methodName;
			const_cast<spi::LocationInfo&>(getLocationInformation()) = spi::LocationInfo(
				m_FilePathNameFromStream.c_str(),
				m_MethodNameFromStream.c_str(),
				std::atoi(lineNumber.c_str())
			);
		}
		LoggingEventEx(const LogString& logger,
			const LevelPtr& level, const LogString& message,
			log4cxx_time_t timeStamp, const LogString& threadName,
			const std::string& fullInfo
		) : spi::LoggingEvent(logger, level, message, spi::LocationInfo())
		{
			// backdoor
			ACCESS((spi::LoggingEvent&)*this, timeStamp) = timeStamp;
			const_cast<LogString&>(ACCESS((spi::LoggingEvent&)*this, threadName)) = threadName;
			// ==
			// const_cast<LogString&>(event->getThreadName()) = threadName;

			m_FilePathNameFromStream = getFileName(fullInfo);
			m_MethodNameFromStream = getMethodName(fullInfo);
			std::string lineNumber = getLineNumber(fullInfo);

			const_cast<spi::LocationInfo&>(getLocationInformation()) = spi::LocationInfo(
				m_FilePathNameFromStream.c_str(),
				m_MethodNameFromStream.c_str(),
				std::atoi(lineNumber.c_str())
			);
		}
		~LoggingEventEx()
		{
		}

	private:
		std::string m_FilePathNameFromStream;
		std::string m_MethodNameFromStream;
	}; // class LoggingEventEx

	log4cxx::spi::LoggingEventPtr createLoggingEvent(const LoggingEventData& data) noexcept
	{
		log4cxx::spi::LoggingEventPtr event(
			new LoggingEventEx(
				data.m_LoggerName,
				Level::toLevel(data.m_Level),
				data.m_Message,
				data.m_Timestamp,
				data.m_ThreadName,
				data.m_PathName,
				data.m_FuncName,
				data.m_LineNumber
			)
		);

		return event;
	}

	log4cxx::spi::LoggingEventPtr createLoggingEvent(
		const LogString& logger,
		const LevelPtr& level, const LogString& message,
		log4cxx_time_t timeStamp, const LogString& threadName,
		const std::string& fullInfo) noexcept
	{
		log4cxx::spi::LoggingEventPtr event(
			new LoggingEventEx(
				logger,
				level,
				message,
				timeStamp,
				threadName,
				fullInfo
			)
		);

		return event;
	}

}}} // log4cxx::ext::loader
