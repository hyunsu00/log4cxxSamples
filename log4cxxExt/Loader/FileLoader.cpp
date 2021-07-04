// FileLoader.cpp
//
#include <memory> // std::unique_ptr
#include "FileLoader.h"

namespace log4cxx { namespace ext { namespace io {

	ByteBuf loadFile(const char* filename)
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
