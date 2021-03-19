#include "JDataProvider.h"
#include "JDataProviderFactory.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace IODataProviderNamespace;

JDataProviderFactory::JDataProviderFactory() {
}

JDataProviderFactory::~JDataProviderFactory() {
}

std::vector<IODataProvider*>* JDataProviderFactory::create() {
	std::vector<IODataProvider*>* ret = new std::vector<IODataProvider*>();
	ret->push_back(new JDataProvider());
	return ret;
}
