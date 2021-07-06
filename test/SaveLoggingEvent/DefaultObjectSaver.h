// DefaultObjectSaver.h
//
#pragma once
#include <vector> // std::vector
#include "LoggingEvnetEx.h"

namespace log4cxx { namespace ext { namespace saver { namespace Default {

	using ByteBuf = std::vector<char>;
	ByteBuf createLoggingEvent(const LoggingEventData& eventData);

}}}} // log4cxx::ext::saver::Default