#include <sasModelProvider/base/EventTypeRegistry.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <eventmanageruanode.h> // EventManagerUaNode
#include <map>
#include <string>

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

    class EventTypeRegistryPrivate {
        friend class EventTypeRegistry;
    private:
        Logger* log;

        // type => super type
        std::map<UaNodeId, UaNodeId> eventTypes;
        // key => nodeId
        std::map<std::string, UaNodeId> eventFields;

        std::string getKey(const UaNodeId& eventTypeId, OpcUa_UInt32 fieldIndex);
    };

    EventTypeRegistry::EventTypeRegistry() {
        d = new EventTypeRegistryPrivate();
        d->log = LoggerFactory::getLogger("EventTypeRegistry");
    }

    EventTypeRegistry::~EventTypeRegistry() {
        delete d;
    }

    void EventTypeRegistry::registerEventType(const UaNodeId& superType,
            const UaNodeId& newType) {
        EventManagerUaNode::registerEventType(superType, newType);
        d->eventTypes[newType] = superType;
        if (d->log->isDebugEnabled()) {
            d->log->info("Registered event type %s for super type %s",
                    newType.toXmlString().toUtf8(), superType.toXmlString().toUtf8());
        }
    }

    void EventTypeRegistry::registerEventField(const UaNodeId& eventTypeId,
            const UaNodeId& fieldNodeId, const UaQualifiedName& filterName) {
        OpcUa_UInt32 fieldIndex = EventManagerUaNode::registerEventField(filterName);
        d->eventFields[d->getKey(eventTypeId, fieldIndex)] = fieldNodeId;
        if (d->log->isDebugEnabled()) {
            d->log->info("Registered event field %s of event type %s for name '%s' with index %d",
                    fieldNodeId.toXmlString().toUtf8(), eventTypeId.toXmlString().toUtf8(),
                    filterName.toFullString().toUtf8(), fieldIndex);
        }
    }

    const UaNodeId* EventTypeRegistry::getEventFieldNodeId(const UaNodeId& eventTypeId,
            OpcUa_UInt32 fieldIndex) {
        // get nodeId for event type or any of its super types
        const UaNodeId* typeId = &eventTypeId;
        std::map<std::string, UaNodeId>::iterator iterFields;
        while (typeId != NULL) {
            iterFields = d->eventFields.find(d->getKey(*typeId, fieldIndex));
            if (iterFields == d->eventFields.end()) {
                // get super type
                std::map<UaNodeId, UaNodeId>::iterator iterTypes = d->eventTypes.find(*typeId);
                if (iterTypes != d->eventTypes.end()) {
                    typeId = &iterTypes->second;
                } else {
                    // index has not been registered
                    typeId = NULL;
                }
            } else {
                typeId = NULL;
            }
        }
        UaNodeId* ret = NULL;
        // if nodeId has been found
        if (iterFields != d->eventFields.end()) {
            ret = &iterFields->second;
            if (d->log->isDebugEnabled()) {
                d->log->info("Provided event field %s for event type %s and index %d",
                        ret->toXmlString().toUtf8(), eventTypeId.toXmlString().toUtf8(), fieldIndex);
            }
        }
        return ret;
    }

    std::string EventTypeRegistryPrivate::getKey(const UaNodeId& eventTypeId,
            OpcUa_UInt32 fieldIndex) {
        UaString key(eventTypeId.toXmlString());
        key += UaString(":") + UaString::number(fieldIndex, 10);
        return std::string(key.toUtf8());
    }

} // namespace SASModelProviderNamespace
