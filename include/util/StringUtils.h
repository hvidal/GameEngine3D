#ifndef INCLUDE_UTIL_STRINGUTILS_H_
#define INCLUDE_UTIL_STRINGUTILS_H_

#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>


class StringUtils {
public:
	static std::vector<std::string> split(const std::string &str, const std::string &delimiter = "\n") {
	    std::vector<std::string> lines;
	    std::string::size_type pos = 0;
	    std::string::size_type prev = 0;
	    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
	    	std::string s = str.substr(prev, pos - prev);
	    	StringUtils::trim(s);
	    	if (s.length() > 0)
	    		lines.push_back(s);
	        prev = pos + 1;
	    }
    	std::string s = str.substr(prev);
    	StringUtils::trim(s);
    	if (s.length() > 0)
    		lines.push_back(s);
	    return lines;
	}

	// trim from start
	static std::string &ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	// trim from end
	static std::string &rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	// trim from both ends
	static std::string &trim(std::string &s) {
		return ltrim(rtrim(s));
	}

};


#endif /* INCLUDE_UTIL_STRINGUTILS_H_ */
