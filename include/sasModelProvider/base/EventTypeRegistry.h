#ifndef SASMODELPROVIDER_BASE_EVENTTYPEREGISTRY_H_
#define SASMODELPROVIDER_BASE_EVENTTYPEREGISTRY_H_

#include <uanodeid.h> // UaNodeId
#include <uaqualifiedname.h> // UaQualifiedName

namespace SASModelProviderNamespace {

    class EventTypeRegistryPrivate;

    class EventTypeRegistry {
    public:
        EventTypeRegistry();
        virtual ~EventTypeRegistry();

        // Registers an event type.
        void registerEventType(const UaNodeId& superType, const UaNodeId& newType);

        // Registers an event field.
        // Copies of the parameter values are stored internally.
        virtual void registerEventField(const UaNodeId& eventTypeId,
                const UaNodeId& fieldNodeId, const UaQualifiedName& filterName);
        virtual const UaNodeId* getEventFieldNodeId(const UaNodeId& eventTypeId,
                OpcUa_UInt32 fieldIndex);
    private:
        EventTypeRegistry(const EventTypeRegistry&);
        EventTypeRegistry& operator=(const EventTypeRegistry&);

        EventTypeRegistryPrivate* d;
    };

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_EVENTTYPEREGISTRY_H_ */
