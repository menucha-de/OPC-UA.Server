#include "BaIODataProviderFactory.h"
#include "BaIODataProvider.h"

using namespace IODataProviderNamespace;

BaIODataProviderFactory::BaIODataProviderFactory() {
}

BaIODataProviderFactory::~BaIODataProviderFactory() {
}

std::vector<IODataProvider*>* BaIODataProviderFactory::create() {
	std::vector<IODataProvider*>* ret = new std::vector<IODataProvider*>();
	ret->push_back(new BaIODataProvider());
	return ret;
}
