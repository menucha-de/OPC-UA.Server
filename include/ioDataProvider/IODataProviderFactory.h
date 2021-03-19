#ifndef IODATAPROVIDER_IODATAPROVIDERFACTORY_H_
#define IODATAPROVIDER_IODATAPROVIDERFACTORY_H_

#include "IODataProvider.h"
#include <vector>

namespace IODataProviderNamespace {

class IODataProviderFactory {
public:
	IODataProviderFactory();
	virtual ~IODataProviderFactory();

	// Creates IODataProviders.
	// The returned container and its components must be deleted by the caller. The components are
	// responsible for destroying their own sub structures.
	virtual std::vector<IODataProvider*>* create() = 0;
};

} /* namespace IODataProviderNamespace */
#endif /* IODATAPROVIDER_IODATAPROVIDERFACTORY_H_ */
