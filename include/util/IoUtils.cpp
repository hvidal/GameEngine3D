#include <SDL2/SDL_filesystem.h>
#include <sys/stat.h>
#include <fstream>
#include <vector>
#include "Log.h"
#include "IoUtils.h"

#define TEST_ENV 1

bool IoUtils::fileExists(const std::string& filename) {
	struct stat buffer;
	return (stat (filename.c_str(), &buffer) == 0);
}


std::string IoUtils::readFile(const std::string& filename) {
	std::ifstream f(filename);
	if (f.fail()) {
		Log::error("File not found: %s", filename.c_str());
		return nullptr;
	}
	std::string s;
	f.seekg(0, std::ios::end);
	s.reserve(f.tellg());
	f.seekg(0, std::ios::beg);
	s.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	return s;
}


const std::string& IoUtils::getCurrentFolder() {
	static std::string basePath;
	if (basePath.length() == 0) {
		basePath = SDL_GetBasePath();
		Log::info("Folder = %s", basePath.c_str());
#ifdef TEST_ENV
		auto pos = basePath.rfind("/Release/");
		basePath = basePath.erase(pos);
#endif
	}
	return basePath;
}


const std::string IoUtils::resource(const std::string& path) {
	return getCurrentFolder() + "/res" + path;
}

