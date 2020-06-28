#ifndef IOUTILS_H_
#define IOUTILS_H_

#include <string>

class IoUtils {
public:
	static bool fileExists(const std::string& filename);
	static std::string readFile(const std::string& filename);
	static const std::string& getCurrentFolder();
	static const std::string resource(const std::string& path);
};

#endif
