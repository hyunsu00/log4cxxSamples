// FileLoader.h
//
#pragma once
#include <vector> // std::vector

namespace log4cxx { namespace ext { namespace io {

    using ByteBuf = std::vector<char>;
    ByteBuf loadFile(const char* filename);

}}} // log4cxx::ext::io
