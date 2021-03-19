#ifndef IODATAPROVIDER_SUBSCRIBERCALLBACK_H_
#define IODATAPROVIDER_SUBSCRIBERCALLBACK_H_

#include "Event.h"

namespace IODataProviderNamespace {

    class SubscriberCallback {
    public:
        SubscriberCallback();
        virtual ~SubscriberCallback();

        // References to parameter values must not be saved in the implementation.
        virtual void valuesChanged(const Event& event) /* throws SubscriberCallbackException */ = 0;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_SUBSCRIBERCALLBACK_H_ */
