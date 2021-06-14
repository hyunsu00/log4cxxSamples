// SocketInputStream.cpp
#ifdef _WIN32
#	include <winsock2.h> // SOCKET, recv
#	include <crtdbg.h> // _ASSERTE
#else
#include <unistd.h>
typedef int SOCKET;
#	include <assert.h>
#	define _ASSERTE assert
#endif
#include <log4cxx/log4cxx.h> // log4cxx_int64_t
#include "InputStreamDef.h"
#include "SocketInputStream.h"
#include <log4cxx/helpers/charsetdecoder.h> // log4cxx::helpers::CharsetDecoder
#include <log4cxx/helpers/bytebuffer.h> // log4cxx::helpers::ByteBuffer
#include <memory> // std::unique_ptr
#include <memory.h> // memcmp

namespace log4cxx { namespace ext { namespace io {

	bool read(SOCKET socket, void* buf, size_t len) noexcept
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

	bool readByte(SOCKET socket, unsigned char& value) noexcept
	{
		return read(socket, &value, sizeof(unsigned char));
	}

	bool readBytes(SOCKET socket, size_t bytes, std::vector<unsigned char>& value) noexcept
	{
		value = std::vector<unsigned char>(bytes, 0);
		if (!read(socket, &value[0], bytes)) {
			return false;
		}

		return true;
	}

	bool readInt(SOCKET socket, int& value) noexcept
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

	bool readLong(SOCKET socket, log4cxx_int64_t& value) noexcept
	{
		_ASSERTE(sizeof(log4cxx_int64_t) == 8 && "읽을 데이터의 크기는 8이여야 한다.");
		if (!read(socket, &value, sizeof(log4cxx_int64_t))) {
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

	bool readUTFString(SOCKET socket, std::string& value, bool skipTypeClass /*= false*/) noexcept
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

	bool readLogString(SOCKET socket, LogString& value, bool skipTypeClass /*= false*/) noexcept
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

	bool readObject(SOCKET socket, MDC::Map& value, bool skipTypeClass /*= false*/) noexcept
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
		SOCKET socket,
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

	bool readLocationInfo(SOCKET socket, std::string& value) noexcept
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
	
	bool readMDC(SOCKET socket, MDC::Map& value) noexcept
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

	bool readNDC(SOCKET socket, LogString& value) noexcept
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

}}} // // log4cxx::ext::io
