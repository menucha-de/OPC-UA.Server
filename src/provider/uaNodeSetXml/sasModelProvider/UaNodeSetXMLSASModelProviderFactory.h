#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_UANODESETXMLSASMODELPROVIDERFACTORY_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_UANODESETXMLSASMODELPROVIDERFACTORY_H_

#include <sasModelProvider/SASModelProviderFactory.h>

class UaNodeSetXMLSASModelProviderFactory: public SASModelProviderNamespace::SASModelProviderFactory {
public:
	UaNodeSetXMLSASModelProviderFactory();
	virtual ~UaNodeSetXMLSASModelProviderFactory();

	// interface SASModelProviderFactory
	virtual std::vector<SASModelProviderNamespace::SASModelProvider*>* create();
};

//Define functions with C symbols (create/destroy SASModelProviderNamespace::SASModelProviderFactory instance).

extern "C" SASModelProviderNamespace::SASModelProviderFactory* createSASModelProviderFactory() {
    return new UaNodeSetXMLSASModelProviderFactory;
}

extern "C" void destroySASModelProviderFactory(SASModelProviderNamespace::SASModelProviderFactory* factory) {
    delete factory;
}
#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_UANODESETXMLSASMODELPROVIDERFACTORY_H_ */
