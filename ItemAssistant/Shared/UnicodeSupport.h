#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <tchar.h>

namespace std {
   typedef basic_string<TCHAR> tstring;
   typedef basic_stringstream<TCHAR> tstringstream;
   typedef basic_ostringstream<TCHAR> tostringstream;
   typedef basic_ostream<TCHAR> tostream;
   typedef basic_ofstream<TCHAR> tofstream;
}

typedef std::tstring tpath;

// Conversion helpers (Implementations are in UnicodeSupport.cpp)
std::string to_ascii_copy(std::wstring const& input);
std::string to_ascii_copy(std::string const& input);
std::string to_utf8_copy(std::tstring const& input);
std::tstring from_ascii_copy(std::string const& input);
std::tstring from_utf8_copy(std::string const& input);

#define STREAM2STR( streamdef ) \
    (((std::tostringstream&)(std::tostringstream().flush() << streamdef)).str())
