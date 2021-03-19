#ifndef SASMODELPROVIDER_BASE_IODATAPROVIDERSUBSCRIBERCALLBACK_H_
#define SASMODELPROVIDER_BASE_IODATAPROVIDERSUBSCRIBERCALLBACK_H_

#include "HaNodeManager.h"
#include <ioDataProvider/SubscriberCallback.h>

namespace SASModelProviderNamespace {

class IODataProviderSubscriberCallbackPrivate;

class IODataProviderSubscriberCallback: public IODataProviderNamespace::SubscriberCallback {
public:
	IODataProviderSubscriberCallback(HaNodeManager& haNodeManager);
	virtual ~IODataProviderSubscriberCallback();

	// interface SubscriberCallback
	virtual void valuesChanged(
			const IODataProviderNamespace::Event& event) /* throws SubscriberCallbackException */;
private:
	IODataProviderSubscriberCallback(const IODataProviderSubscriberCallback&);
	IODataProviderSubscriberCallback& operator=(
			const IODataProviderSubscriberCallback&);

	IODataProviderSubscriberCallbackPrivate* d;
};

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_IODATAPROVIDERSUBSCRIBERCALLBACK_H_ */
