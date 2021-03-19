#ifndef PROVIDER_BINARY_IODATAPROVIDER_JDATAPROVIDERFACTORY_H
#define PROVIDER_BINARY_IODATAPROVIDER_JDATAPROVIDERFACTORY_H

#include <ioDataProvider/IODataProviderFactory.h>
#include <vector>

class JDataProviderFactory : public IODataProviderNamespace::IODataProviderFactory {
public:
	JDataProviderFactory();
    virtual ~JDataProviderFactory();

    // interface IODataProviderFactory
    virtual std::vector<IODataProviderNamespace::IODataProvider*>* create();
};

// Define functions with C symbols 
// (create/destroy IODataProviderNamespace::IODataProviderFactory instance).

extern "C" IODataProviderNamespace::IODataProviderFactory* createIODataProviderFactory() {
    return new JDataProviderFactory();
}

extern "C" void destroyIODataProviderFactory(IODataProviderNamespace::IODataProviderFactory* factory) {
    delete factory;
}
#endif /* PROVIDER_BINARY_IODATAPROVIDER_JDATAPROVIDERFACTORY_H */
