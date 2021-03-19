#ifndef SASMODELPROVIDER_SASMODELPROVIDERFACTORY_H_
#define SASMODELPROVIDER_SASMODELPROVIDERFACTORY_H_

#include "SASModelProvider.h"
#include <vector>

namespace SASModelProviderNamespace {

class SASModelProviderFactory {
public:
	SASModelProviderFactory();
	virtual ~SASModelProviderFactory();

	// Creates SASModelProviders.
	// The returned container and its components must be deleted by the caller. The components are
	// responsible for destroying their own sub structures.
	virtual std::vector<SASModelProvider*>* create() = 0;
};

} /* namespace SASModelProviderNamespace */
#endif /* SASMODELPROVIDER_SASMODELPROVIDERFACTORY_H_ */
