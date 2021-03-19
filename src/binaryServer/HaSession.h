#ifndef BINARYSERVER_HASESSION_H
#define BINARYSERVER_HASESSION_H

#include "Event.h"
#include "EventField.h"
#include "NodeAttributes.h"
#include <uaargument.h> // UaArguments
#include <uaarraytemplates.h> // UaStringArray
#include <uanodeid.h> // UaNodeId
#include <map>
#include <vector>

class HaSessionPrivate;

class HaSession {
public:

    class Configuration {
    public:
        std::string host;
        int port;
        std::string* username;
        std::string* password;
        int connectTimeout;
        int sendReceiveTimeout;
        int maxReconnectDelay;
        int publishingInterval;
    };

    class HaSessionCallback {
    public:
        HaSessionCallback();
        virtual ~HaSessionCallback();

        virtual void dataChanged(std::vector<NodeAttributes*>& nodeAttributes)
        /* throws Exception */ = 0;
        virtual void newEvents(std::vector<const BinaryServerNamespace::Event*>& events)
        /* throws Exception */ = 0;
    };

    HaSession(Configuration& conf, HaSessionCallback& callback) /*throws MutexException*/;
    virtual ~HaSession() /* throws HaSessionException */;

    virtual void open() /* throws HaSessionException */;
    virtual void close() /* throws HaSessionException, HaSubscriptionException */;

    virtual UaStringArray getNamespaceTable() const;

    virtual UaStructureDefinition getStructureDefinition(const UaNodeId& dataTypeId);
    // Returns the super types incl. the first one in namespace 0 starting with the nearest parent.
    // The returned container must be deleted by the caller.
    // The components are responsible for destroying their own sub structures.
    virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& typeId) /*throws HaSessionException*/;
    virtual void getMethodArguments(const UaNodeId& methodId, UaArguments& returnInputArguments,
            UaArguments& returnOutputArguments) /*throws HaSessionException*/;

    // The returned components are responsible for destroying their own sub structures.
    virtual void read(const std::vector<const UaNodeId*>& nodeIds,
            std::vector<NodeAttributes*>& returnNodeAttributes) /*throws HaSessionException*/;
    virtual void write(const std::map<const UaNodeId*, const UaVariant*>& nodes) /* throws HaSessionException */;
    // The returned container and its components must be deleted by the caller.
    // The components are responsible for destroying their own sub structures.
    virtual std::vector<UaVariant*>* call(const UaNodeId& methodId, const UaNodeId& objectId,
            const std::vector<UaVariant*>& params) /*throws HaSessionException*/;
    virtual void subscribe(const std::vector<const UaNodeId*>& nodeIds,
            std::vector<NodeAttributes*>& returnNodeAttributes,
            std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>& returnEventFields) /* throws HaSubscriptionException */;
    virtual void unsubscribe(const std::vector<const UaNodeId*>& nodeIds) /* throws HaSubscriptionException */;
    // Deletes all subscriptions.
    virtual void unsubscribe() /* throws HaSubscriptionException */;
private:
    HaSession(const HaSession& orig);
    HaSession& operator=(const HaSession&);

    HaSessionPrivate* d;
};

#endif /* BINARYSERVER_HASESSION_H */

