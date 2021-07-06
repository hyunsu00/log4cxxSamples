// DefaultObjectSaver.cpp
#include "DefaultObjectSaver.h"
#include "DefaultInputStreamDef.h"
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer 
#include <log4cxx/helpers/charsetencoder.h> // 
namespace log4cxx { namespace ext { namespace saver { namespace Default {

	void writeLong(ByteBuf& byteBuf, log4cxx_int64_t value)
	{
		_ASSERTE(sizeof(log4cxx_int64_t) == 8 && "읽을 데이터의 크기는 8이여야 한다.");

		// 빅 엔디안으로 저장
		char bytes[8];
		bytes[7] = (char)(value & 0xFF);
		bytes[6] = (char)((value >> 8) & 0xFF);
		bytes[5] = (char)((value >> 16) & 0xFF);
		bytes[4] = (char)((value >> 24) & 0xFF);
		bytes[3] = (char)((value >> 32) & 0xFF);
		bytes[2] = (char)((value >> 40) & 0xFF);
		bytes[1] = (char)((value >> 48) & 0xFF);
		bytes[0] = (char)((value >> 56) & 0xFF);

		byteBuf.insert(byteBuf.end(), bytes, bytes + sizeof(bytes));
	}

	void writeInt(ByteBuf& byteBuf, int value)
	{
		_ASSERTE(sizeof(int) == 4 && "읽을 데이터의 크기는 4이여야 한다.");

		// 빅 엔디안으로 저장
		char bytes[4];
		bytes[3] = (char)(value & 0xFF);
		bytes[2] = (char)((value >> 8) & 0xFF);
		bytes[1] = (char)((value >> 16) & 0xFF);
		bytes[0] = (char)((value >> 24) & 0xFF);

		byteBuf.insert(byteBuf.end(), bytes, bytes + sizeof(bytes));
	}

	void writeShort(ByteBuf& byteBuf, short value)
	{
		_ASSERTE(sizeof(short) == 2 && "읽을 데이터의 크기는 2이여야 한다.");

		// 빅 엔디안으로 저장
		char bytes[2];
		bytes[1] = (char)(value & 0xFF);
		bytes[0] = (char)((value >> 8) & 0xFF);

		byteBuf.insert(byteBuf.end(), bytes, bytes + sizeof(bytes));
	}

	ByteBuf getFullInfo(const std::string& funcName, const std::string& pathName, const std::string& lineNumber)
	{
		std::string fullInfo(funcName);
		size_t openParen = fullInfo.find('(');
		if (openParen != std::string::npos) {
			size_t space = fullInfo.find(' ');
			if (space != std::string::npos && space < openParen) {
				fullInfo.erase(0, space + 1);
			}
		}
		openParen = fullInfo.find('(');
		if (openParen != std::string::npos) {
			size_t classSep = fullInfo.rfind("::", openParen);
			if (classSep != std::string::npos) {
				fullInfo.replace(classSep, 2, ".");
			}
			else {
				fullInfo.insert(0, ".");
			}
		}
		fullInfo.append(1, '(');
		fullInfo.append(pathName);
		fullInfo.append(1, ':');
		fullInfo.append(lineNumber);
		fullInfo.append(1, ')');

		return ByteBuf(fullInfo.c_str(), fullInfo.c_str() + fullInfo.size());
	}

	ByteBuf toUTF8(const LogString& value)
	{
		using namespace log4cxx::helpers;
		CharsetEncoderPtr utf8Encoder(CharsetEncoder::getUTF8Encoder());

		size_t maxSize = 6 * value.size();
		ByteBuf data(maxSize, 0);
		ByteBuffer dataBuf(&data[0], maxSize);
		LogString::const_iterator iter(value.begin());
		utf8Encoder->encode(value, iter, dataBuf);
		dataBuf.flip();
		size_t len = dataBuf.limit();
		data.erase(data.begin() + len, data.end());

		return data;
	}

	ByteBuf createLoggingEvent(const LoggingEventData& eventData)
	{
		using namespace log4cxx::ext::io::Default;

		ByteBuf byteBuf;
		
		// writeProlog(os, p);
		byteBuf.push_back(TC_OBJECT);
		{	
			// TC_CLASSDESC
			byteBuf.insert(byteBuf.end(), classDesc::LOGGING_EVENT, classDesc::LOGGING_EVENT + sizeof(classDesc::LOGGING_EVENT));
		}

		// os.writeBytes(lookupsRequired, sizeof(lookupsRequired), p);
		{
			unsigned char lookupsRequired[] = { 0, 0 };
			byteBuf.insert(byteBuf.end(), lookupsRequired, lookupsRequired + sizeof(lookupsRequired));
		}

		// os.writeLong(timeStamp/1000, p);
		log4cxx_int64_t timeStamp = eventData.m_Timestamp;
		{
			writeLong(byteBuf, timeStamp / 1000);
		}

		// os.writeObject(logger, p);
		ByteBuf logger = toUTF8(eventData.m_LoggerName);
		{
			byteBuf.push_back(TC_STRING);
			writeShort(byteBuf, static_cast<short>(logger.size()));
			byteBuf.insert(byteBuf.end(), &logger[0], &logger[0] + logger.size());
		}

		// locationInfo.write(os, p);
		{
			byteBuf.push_back(TC_OBJECT);
			{
				// TC_CLASSDESC
				byteBuf.insert(byteBuf.end(), classDesc::LOCATION_INFO, classDesc::LOCATION_INFO + sizeof(classDesc::LOCATION_INFO));
			}

			ByteBuf fullInfo = getFullInfo(eventData.m_FuncName, eventData.m_PathName, eventData.m_LineNumber);
			{
				byteBuf.push_back(TC_STRING);
				writeShort(byteBuf, static_cast<short>(fullInfo.size()));
				byteBuf.insert(byteBuf.end(), &fullInfo[0], &fullInfo[0] + fullInfo.size());
			}
		}

		// mdc
		{
			byteBuf.push_back(TC_NULL);
		}

		// ndc
		{
			byteBuf.push_back(TC_NULL);
		}

		// os.writeObject(message, p);
		ByteBuf message = toUTF8(eventData.m_Message);
		{
			byteBuf.push_back(TC_STRING);
			writeShort(byteBuf, static_cast<short>(message.size()));
			byteBuf.insert(byteBuf.end(), &message[0], &message[0] + message.size());
		}

		// os.writeObject(threadName, p);
		ByteBuf threadName = toUTF8(eventData.m_ThreadName);
		{
			byteBuf.push_back(TC_STRING);
			writeShort(byteBuf, static_cast<short>(threadName.size()));
			byteBuf.insert(byteBuf.end(), &threadName[0], &threadName[0] + threadName.size());
		}

		// os.writeNull(p);
		{
			byteBuf.push_back(TC_NULL);
		}

		// os.writeByte(ObjectOutputStream::TC_BLOCKDATA, p);
		{
			byteBuf.push_back(TC_BLOCKDATA);
		}

		// os.writeByte(0x04, p);
		{
			byteBuf.push_back(0x04);
		}

		// os.writeInt(level->toInt(), p);
		int level = eventData.m_Level;
		{
			writeInt(byteBuf, level);
		}

		// os.writeNull(p);
		{
			byteBuf.push_back(TC_NULL);
		}

		// os.writeByte(ObjectOutputStream::TC_ENDBLOCKDATA, p);
		{
			byteBuf.push_back(TC_ENDBLOCKDATA);
		}

		return byteBuf;
	}

}}}} // log4cxx::ext::saver::Default