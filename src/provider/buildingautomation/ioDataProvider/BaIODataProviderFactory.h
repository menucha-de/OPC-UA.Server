#ifndef PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BAIODATAPROVIDERFACTORY_H_
#define PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BAIODATAPROVIDERFACTORY_H_

#include <ioDataProvider/IODataProviderFactory.h>
#include <vector>

class BaIODataProviderFactory : public IODataProviderNamespace::IODataProviderFactory {
public:
    BaIODataProviderFactory();
    virtual ~BaIODataProviderFactory();

    // interface IODataProviderFactory
    virtual std::vector<IODataProviderNamespace::IODataProvider*>* create();
};


//Define functions with C symbols (create/destroy IODataProviderNamespace::IODataProviderFactory instance).

extern "C" IODataProviderNamespace::IODataProviderFactory* createIODataProviderFactory() {
    return new BaIODataProviderFactory();
}

extern "C" void destroyIODataProviderFactory(IODataProviderNamespace::IODataProviderFactory* factory) {
    delete factory;
}

#endif /* PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BAIODATAPROVIDERFACTORY_H_ */
