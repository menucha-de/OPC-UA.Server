#ifndef PROVIDER_BINARY_IODATAPROVIDER_BINARYIODATAPROVIDERFACTORY_H
#define PROVIDER_BINARY_IODATAPROVIDER_BINARYIODATAPROVIDERFACTORY_H

#include <ioDataProvider/IODataProviderFactory.h>
#include <vector>

class BinaryIODataProviderFactory : public IODataProviderNamespace::IODataProviderFactory {
public:
    BinaryIODataProviderFactory();
    virtual ~BinaryIODataProviderFactory();

    // interface IODataProviderFactory
    virtual std::vector<IODataProviderNamespace::IODataProvider*>* create();
};

// Define functions with C symbols 
// (create/destroy IODataProviderNamespace::IODataProviderFactory instance).

extern "C" IODataProviderNamespace::IODataProviderFactory* createIODataProviderFactory() {
    return new BinaryIODataProviderFactory();
}

extern "C" void destroyIODataProviderFactory(IODataProviderNamespace::IODataProviderFactory* factory) {
    delete factory;
}
#endif /* PROVIDER_BINARY_IODATAPROVIDER_BINARYIODATAPROVIDERFACTORY_H */
