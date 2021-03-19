#include "EventField.h"

namespace BinaryServerNamespace {

    using namespace CommonNamespace;

    class EventFieldPrivate {
        friend class EventField;
    private:
        const UaNodeId* nodeId;
        const UaQualifiedName* qName;
        const UaNodeId* dataTypeId;
        const UaVariant* value;
        Exception* exception;
        bool hasAttachedValues;
    };

    EventField::EventField(const UaNodeId& nodeId, bool attachValues) {
        d = new EventFieldPrivate();
        d->nodeId = &nodeId;
        d->qName = NULL;
        d->dataTypeId = NULL;
        d->value = NULL;
        d->exception = NULL;
        d->hasAttachedValues = attachValues;
    }

    EventField::EventField(const EventField& orig) {
        // avoid self-assignment
        if (this == &orig) {
            return;
        }
        d = new EventFieldPrivate();
        d->nodeId = new UaNodeId(*orig.d->nodeId);
        d->qName = orig.d->qName == NULL ? NULL : new UaQualifiedName(*orig.d->qName);
        d->dataTypeId = orig.d->dataTypeId == NULL ? NULL : new UaNodeId(*orig.d->dataTypeId);
        d->value = orig.d->value == NULL ? NULL : new UaVariant(*orig.d->value);
        d->exception = orig.d->exception == NULL ? NULL : new Exception(*orig.d->exception);
        d->hasAttachedValues = true;
    }

    EventField::~EventField() {
        if (d->hasAttachedValues) {
            delete d->nodeId;
            delete d->qName;
            delete d->dataTypeId;
            delete d->value;
            delete d->exception;
        }
        delete d;
    }

    const UaNodeId& EventField::getNodeId() const {
        return *d->nodeId;
    }

    void EventField::setNodeId(const UaNodeId& nodeId) {
        if (d->hasAttachedValues) {
            delete d->nodeId;
        }
        d->nodeId = &nodeId;
    }

    const UaQualifiedName* EventField::getQualifiedName() const {
        return d->qName;
    }

    void EventField::setQualifiedName(const UaQualifiedName* qName) {
        if (d->hasAttachedValues) {
            delete d->qName;
        }
        d->qName = qName;
    }

    const UaNodeId* EventField::getDataTypeId() const {
        return d->dataTypeId;
    }

    void EventField::setDataTypeId(const UaNodeId* dataTypeId) {
        if (d->hasAttachedValues) {
            delete d->dataTypeId;
        }
        d->dataTypeId = dataTypeId;
    }

    const UaVariant* EventField::getValue() const {
        return d->value;
    }

    void EventField::setValue(const UaVariant* value) {
        if (d->hasAttachedValues) {
            delete d->value;
        }
        d->value = value;
    }

    Exception* EventField::getException() const {
        return d->exception;
    }

    void EventField::setException(CommonNamespace::Exception* exception) {
        if (d->hasAttachedValues) {
            delete d->exception;
        }
        d->exception = exception;
    }
} // BinaryServerNamespace