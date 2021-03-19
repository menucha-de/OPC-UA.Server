#ifndef PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BASASMODELPROVIDERFACTORY_H_
#define PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BASASMODELPROVIDERFACTORY_H_

#include <sasModelProvider/SASModelProviderFactory.h>

class BaSASModelProviderFactory : public SASModelProviderNamespace::SASModelProviderFactory {
public:
    BaSASModelProviderFactory();
    virtual ~BaSASModelProviderFactory();

    // interface SASModelProviderFactory
    virtual std::vector<SASModelProviderNamespace::SASModelProvider*>* create();
};

//Define functions with C symbols (create/destroy SASModelProviderNamespace::SASModelProviderFactory instance).

extern "C" SASModelProviderNamespace::SASModelProviderFactory* createSASModelProviderFactory() {
    return new BaSASModelProviderFactory();
}

extern "C" void destroySASModelProviderFactory(SASModelProviderNamespace::SASModelProviderFactory* factory) {
    delete factory;
}
#endif /* PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BASASMODELPROVIDERFACTORY_H_ */
