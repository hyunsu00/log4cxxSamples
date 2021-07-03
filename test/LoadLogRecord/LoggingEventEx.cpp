// LoggingEventEx.cpp
//
#include <log4cxx/spi/loggingevent.h> // log4cxx::spi::LoggingEventPtr
#include "LoggingEvnetEx.h"

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

namespace log4cxx { namespace ext { namespace loader {
	
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
			ACCESS((spi::LoggingEvent&)*this, timeStamp) = timeStamp;
			const_cast<LogString&>(ACCESS((spi::LoggingEvent&)*this, threadName)) = threadName;

			m_FilePathNameFromStream = filePathName;
			m_MethodNameFromStream = methodName;
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

}}} // log4cxx::ext::loader
