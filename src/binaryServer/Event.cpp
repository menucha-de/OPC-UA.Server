#include "Event.h"
#include <common/VectorScopeGuard.h>

namespace BinaryServerNamespace {

    using namespace CommonNamespace;

    class EventPrivate {
        friend class Event;
    private:
        const UaNodeId* eventTypeId;
        std::vector<EventField*>* eventFields;
        bool hasAttachedValues;
    };

    Event::Event(const UaNodeId& eventTypeId, std::vector<EventField*>& eventFields,
            bool attachValues) {
        d = new EventPrivate();
        d->eventTypeId = &eventTypeId;
        d->eventFields = &eventFields;
        d->hasAttachedValues = attachValues;
    }

    Event::~Event() {
        if (d->hasAttachedValues) {
            delete d->eventTypeId;
            VectorScopeGuard<EventField> eventFieldsSG(d->eventFields);
        }
        delete d;
    }

    void Event::setEventTypeId(const UaNodeId& eventTypeId) {
        if (d->hasAttachedValues) {
            delete d->eventTypeId;
        }
        d->eventTypeId = &eventTypeId;
    }

    const UaNodeId& Event::getEventTypeId() const {
        return *d->eventTypeId;
    }

    std::vector<EventField*>* Event::getEventFields() const {
        return d->eventFields;
    }

    void Event::setEventFields(std::vector<EventField*>* eventFields) {
        if (d->hasAttachedValues) {
            VectorScopeGuard<EventField> eventFieldsSG(d->eventFields);
        }
        d->eventFields = eventFields;
    }
} // BinaryServerNamespace