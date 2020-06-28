#ifndef GAMEDEV3D_SERIALIZER_H
#define GAMEDEV3D_SERIALIZER_H

#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <LinearMath/btVector3.h>
#include "Interfaces.h"


class Serializer: public ISerializer {
private:
	std::string mFilename;
	std::fstream mFile;

	std::unordered_map<std::string,Factory> mFactoryMap;

public:
	Serializer(const std::string&);
	~Serializer();

	virtual void open(bool createFile = false) override;
	virtual void close() override;

	virtual void writeBegin(const std::string&, unsigned long) override;
	virtual void write(const btVector3&) override;
	virtual void write(const std::string&) override;
	virtual void write(bool) override;
	virtual void write(int) override;
	virtual void write(unsigned int) override;
	virtual void write(unsigned long) override;
	virtual void write(float) override;
	virtual void write(const std::vector<unsigned int>&) override;

	virtual bool readBegin(std::string&, unsigned long&) override;
	virtual void read(btVector3&) override;
	virtual void read(std::string&) override;
	virtual void read(bool&) override;
	virtual void read(int&) override;
	virtual void read(unsigned int&) override;
	virtual void read(unsigned long&) override;
	virtual void read(float&) override;
	virtual void read(std::vector<unsigned int>&) override;

	virtual void addFactory(std::pair<std::string,Factory>) override;
	virtual const Factory& getFactory(const std::string&) const override;
};

//-----------------------------------------------------------------------------

#endif
