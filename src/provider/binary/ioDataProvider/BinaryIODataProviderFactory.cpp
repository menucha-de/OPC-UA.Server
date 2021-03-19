#include "BinaryIODataProviderFactory.h"
#include "BinaryIODataProvider.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace IODataProviderNamespace;

BinaryIODataProviderFactory::BinaryIODataProviderFactory() {
}

BinaryIODataProviderFactory::~BinaryIODataProviderFactory() {
}

std::vector<IODataProvider*>* BinaryIODataProviderFactory::create() {
	std::vector<IODataProvider*>* ret = new std::vector<IODataProvider*>();
	ret->push_back(new BinaryIODataProvider());
	return ret;
}
