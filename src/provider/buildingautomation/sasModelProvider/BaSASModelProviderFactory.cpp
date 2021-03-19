#include "BaSASModelProviderFactory.h"
#include "BaSASModelProvider.h"

BaSASModelProviderFactory::BaSASModelProviderFactory() {
}

BaSASModelProviderFactory::~BaSASModelProviderFactory() {
}

std::vector<SASModelProviderNamespace::SASModelProvider*>* BaSASModelProviderFactory::create() {
	std::vector<SASModelProviderNamespace::SASModelProvider*>* ret =
			new std::vector<SASModelProviderNamespace::SASModelProvider*>();
	ret->push_back(new BaSASModelProvider());
	return ret;
}
