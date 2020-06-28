#include <sstream>
#include "Serializer.h"


template<typename T>
constexpr void writeValue(std::fstream& f, const T& v) {
	f.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

template<>
constexpr void writeValue(std::fstream& f, const std::string& v) {
	std::string::size_type len = v.length();
	f.write(reinterpret_cast<const char*>(&len), sizeof(len));
	f.write(v.c_str(), len * sizeof(char));
}

//-----------------------------------------------------------------------------

template<typename T>
constexpr void readValue(std::fstream& f, T& v) {
	f.read(reinterpret_cast<char*>(&v), sizeof(v));
}

template<>
void readValue(std::fstream& f, std::string& v) {
	std::string::size_type len;
	f.read(reinterpret_cast<char*>(&len), sizeof(len));

	char buffer[len + 1];
	f.read(buffer, len);
	buffer[len] = '\0';
	v = buffer;
}

//-----------------------------------------------------------------------------

void Serializer::addFactory(std::pair<std::string,Factory> pair) {
	mFactoryMap.insert(pair);
}


const Factory& Serializer::getFactory(const std::string& className) const {
	if (mFactoryMap.find(className) != mFactoryMap.end()) {
		return mFactoryMap.at(className);
	} else
		throw std::runtime_error("No factory found for: " + className);
}

//-----------------------------------------------------------------------------


Serializer::Serializer(const std::string& filename):
	mFilename(filename)
{}


Serializer::~Serializer() {
	close();
}


void Serializer::open(bool createFile) {
	if (createFile) {
		std::ofstream out;
		out.open(mFilename);
		out << "1" << std::endl;
		out.close();
	}
	mFile.open(mFilename, std::ios::in | std::ios::out);
}


void Serializer::close() {
	if (mFile.is_open()) {
		mFile.flush();
		mFile.close();
	}
}


static const std::string BEGIN_("BEGIN_");

void Serializer::writeBegin(const std::string& s, unsigned long objectId) {
	writeValue(mFile, BEGIN_ + s);
	writeValue(mFile, objectId);
}


void Serializer::write(const btVector3& v) {
	writeValue(mFile, v.x());
	writeValue(mFile, v.y());
	writeValue(mFile, v.z());
}


void Serializer::write(bool b) {
	writeValue(mFile, b);
}


void Serializer::write(int i) {
	writeValue(mFile, i);
}


void Serializer::write(unsigned int ui) {
	writeValue(mFile, ui);
}


void Serializer::write(unsigned long l) {
	writeValue(mFile, l);
}


void Serializer::write(float d) {
	writeValue(mFile, d);
}


void Serializer::write(const std::string& s) {
	writeValue(mFile, s);
}


void Serializer::write(const std::vector<unsigned int>& v) {
	std::vector<unsigned int>::size_type size = v.size();
	writeValue(mFile, size);
	for (unsigned int ui : v)
		writeValue(mFile, ui);
}

//-----------------------------------------------------------------------------

bool Serializer::readBegin(std::string& className, unsigned long& objectId) {
	std::string s;
	readValue(mFile, s);

	if (s == "END")
		return false;

	char name[99];
	int result = sscanf(s.c_str(), "BEGIN_%s", name);
	if (result == 1) {
		className = name;
		readValue(mFile, objectId);
		return true;
	}
	throw std::runtime_error("BEGIN not found: " + s);
}


void Serializer::read(btVector3& v) {
	float x, y, z;
	readValue(mFile, x);
	readValue(mFile, y);
	readValue(mFile, z);
	v.setValue(x, y, z);
}


void Serializer::read(float& v) {
	readValue(mFile, v);
}


void Serializer::read(unsigned int& v) {
	readValue(mFile, v);
}


void Serializer::read(unsigned long& v) {
	readValue(mFile, v);
}


void Serializer::read(int& v) {
	readValue(mFile, v);
}


void Serializer::read(bool& b) {
	readValue(mFile, b);
}


void Serializer::read(std::string& v) {
	readValue(mFile, v);
}


void Serializer::read(std::vector<unsigned int>& v) {
	std::vector<unsigned int>::size_type size;
	readValue(mFile, size);
	v.clear();
	v.reserve(size);
	unsigned int ui;
	for (auto i = 0; i < size; ++i) {
		readValue(mFile, ui);
		v.push_back(ui);
	}
}

