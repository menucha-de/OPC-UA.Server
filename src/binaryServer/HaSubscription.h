#ifndef BINARYSERVER_HASUBSCRIPTION_H
#define BINARYSERVER_HASUBSCRIPTION_H

#include "Event.h"
#include "EventField.h"
#include "HaSession.h"
#include <uasession.h> // UaSession
#include <uanodeid.h> // UaNodeId
#include <vector>

class HaSubscriptionPrivate;

class HaSubscription {
public:

    class HaSubscriptionCallback {
    public:
        HaSubscriptionCallback();
        virtual ~HaSubscriptionCallback();

        virtual void dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
        /* throws Exception */ = 0;
        virtual void newEvents(std::vector<const BinaryServerNamespace::Event*>& events)
        /* throws Exception */ = 0;
    private:
    };

    // sendReceiveTimeout: in sec.
    // publishingInterval: in ms
    HaSubscription(UaClientSdk::UaSession& session, int sendReceiveTimeout, int publishingInterval,
            HaSubscriptionCallback& callback) /* throws MutexException */;
    virtual ~HaSubscription() /* throws HaSubscriptionException */;

    void add(std::vector<NodeAttributes*>& nodeAttributes,
            std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>& eventFields)
    /* throws HaSubscriptionException */;
    void remove(const std::vector<const UaNodeId*>& nodeIds) /* throws HaSubscriptionException */;
    void removeAll() /* throws HaSubscriptionException */;

    std::vector<const UaNodeId*>* getMonitoredItems() /* throws HaSubscriptionException */;
private:
    HaSubscription(const HaSubscription& orig);
    HaSubscription& operator=(const HaSubscription&);

    HaSubscriptionPrivate* d;
};

#endif /* BINARYSERVER_HASUBSCRIPTION_H */

