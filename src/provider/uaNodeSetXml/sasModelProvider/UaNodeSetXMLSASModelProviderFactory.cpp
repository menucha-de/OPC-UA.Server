#include "UaNodeSetXMLSASModelProviderFactory.h"
#include "UaNodeSetXMLSASModelProvider.h"
#include <sasModelProvider/SASModelProvider.h>
#include <vector>

using namespace SASModelProviderNamespace;

UaNodeSetXMLSASModelProviderFactory::UaNodeSetXMLSASModelProviderFactory() {
}

UaNodeSetXMLSASModelProviderFactory::~UaNodeSetXMLSASModelProviderFactory() {
}

std::vector<SASModelProvider*>* UaNodeSetXMLSASModelProviderFactory::create() {
	std::vector<SASModelProvider*>* ret = new std::vector<SASModelProvider*>();
	ret->push_back(new UaNodeSetXMLSASModelProvider());
	return ret;
}
