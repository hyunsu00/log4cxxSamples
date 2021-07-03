// LoggingEventEx.h
#pragma once

namespace log4cxx { namespace ext {

	// std::string은 utf-8
	struct LoggingEventData
	{
		LogString m_LoggerName;
		int m_Level;
		LogString m_Message;
		__int64 m_Timestamp;
		LogString m_ThreadName;
		std::string m_PathName;
		std::string m_FuncName;
		std::string m_LineNumber;
	}; // struct LoggingEventData

}} // log4cxx::ext

namespace log4cxx { namespace ext { namespace loader {

	log4cxx::spi::LoggingEventPtr createLoggingEvent(const LoggingEventData& data) noexcept;

}}} // log4cxx::ext::loader
