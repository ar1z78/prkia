#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>
#include <algorithm>
#include <cctype>

namespace StringUtil 
{
	// Inside namespace StringUtil in StringUtil.h
	inline void replace_all_ansi(std::string& str, const std::string& from, const std::string& to) {
		if (from.empty()) return;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}

    // Replaces all occurrences of 'from' with 'to' in-place
    inline void replace_all(std::tstring& str, const std::tstring& from, const std::tstring& to) 
    {
        if (from.empty()) return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::tstring::npos) 
        {
            str.replace(start_pos, from.length(), to);
            // Move start_pos forward to avoid infinite loops if 'to' contains 'from'
            start_pos += to.length();
        }
    }

    // Removes leading whitespace in-place
    inline void trim_left(std::tstring& s) 
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }

    // Removes trailing whitespace in-place
    inline void trim_right(std::tstring& s) 
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // Removes both leading and trailing whitespace in-place
    inline void trim(std::tstring& s) 
    {
        trim_left(s);
        trim_right(s);
    }
	
    inline std::tstring xml_encode(const std::tstring& data) {
        std::tstring buffer;
        buffer.reserve(data.size());
        for (size_t pos = 0; pos != data.size(); ++pos) {
            switch (data[pos]) {
                case '&':  buffer.append(_T("&amp;"));      break;
                case '\"': buffer.append(_T("&quot;"));     break;
                case '\'': buffer.append(_T("&apos;"));     break;
                case '<':  buffer.append(_T("&lt;"));       break;
                case '>':  buffer.append(_T("&gt;"));       break;
                default:   buffer.append(1, data[pos]);     break;
            }
        }
        return buffer;
    }

    // Case-insensitive comparison (Standard C++ / GCC / VS compatible)
    inline bool iequals(const std::tstring& a, const std::tstring& b) {
        if (a.length() != b.length()) return false;
        for (size_t i = 0; i < a.length(); ++i) {
            if (::towlower(a[i]) != ::towlower(b[i])) return false;
        }
        return true;
    }

    // Returns a lower-case copy of the string
    inline std::tstring to_lower(std::tstring s) {
        std::transform(s.begin(), s.end(), s.begin(), [](TCHAR c) { 
            return (TCHAR)::towlower(c); 
        });
        return s;
    }

    // Returns an upper-case copy of the string
    inline std::tstring to_upper(std::tstring s) {
        std::transform(s.begin(), s.end(), s.begin(), [](TCHAR c) { 
            return (TCHAR)::towupper(c); 
        });
        return s;
    }
    // Checks if a string starts with a specific prefix
    inline bool starts_with(const std::tstring& str, const std::tstring& prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    // Checks if a string ends with a specific suffix
    inline bool ends_with(const std::tstring& str, const std::tstring& suffix) {
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    // Removes the specific characters ' \n\r\t' from both ends
    inline void trim_ao(std::tstring& s) 
    {
        auto is_ao_whitespace = [](TCHAR ch) {
            return ch == _T(' ') || ch == _T('\n') || ch == _T('\r') || ch == _T('\t');
        };

        // Trim from start
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](TCHAR ch) {
            return !is_ao_whitespace(ch);
        }));

        // Trim from end
        s.erase(std::find_if(s.rbegin(), s.rend(), [&](TCHAR ch) {
            return !is_ao_whitespace(ch);
        }).base(), s.end());
    }


}


#endif // STRING_UTIL_H
