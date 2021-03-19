#include "NodeAttributes.h"

using namespace CommonNamespace;

class NodeAttributesPrivate {
    friend class NodeAttributes;
private:
    UaNodeId* nodeId;
    UaVariant* value;
    UaNodeId* dataType;
    OpcUa_NodeClass* nodeClass;
    Exception* exception;
    bool hasAttachedValues;
};

NodeAttributes::NodeAttributes(UaNodeId& nodeId, bool attachValues) {
    d = new NodeAttributesPrivate();
    d->nodeId = &nodeId;
    d->value = NULL;
    d->dataType = NULL;
    d->nodeClass = NULL;
    d->exception = NULL;
    d->hasAttachedValues = attachValues;
}

NodeAttributes::NodeAttributes(const NodeAttributes& orig) {
    d = new NodeAttributesPrivate();
    d->nodeId = new UaNodeId(*orig.d->nodeId);
    d->value = orig.d->value == NULL ? NULL : new UaVariant(*orig.d->value);
    d->dataType = orig.d->dataType == NULL ? NULL : new UaNodeId(*orig.d->dataType);
    d->nodeClass = orig.d->nodeClass == NULL ? NULL : new OpcUa_NodeClass(*orig.d->nodeClass);
    d->exception = orig.d->exception == NULL ? NULL : new Exception(*orig.d->exception);
    d->hasAttachedValues = true;
}

NodeAttributes::~NodeAttributes() {
    if (d->hasAttachedValues) {
        delete d->nodeId;
        delete d->value;
        delete d->dataType;
        delete d->exception;
        delete d->nodeClass;
    }
    delete d;
}

UaNodeId& NodeAttributes::getNodeId() const {
    return *d->nodeId;
}

UaVariant* NodeAttributes::getValue() const {
    return d->value;
}

void NodeAttributes::setValue(UaVariant* value) {
    if (d->hasAttachedValues) {
        delete d->value;
    }
    d->value = value;
}

UaNodeId* NodeAttributes::getDataType() const {
    return d->dataType;
}

void NodeAttributes::setDataType(UaNodeId* dataType) {
    if (d->hasAttachedValues) {
        delete d->dataType;
    }
    d->dataType = dataType;
}

OpcUa_NodeClass* NodeAttributes::getNodeClass() const {
    return d->nodeClass;
}

void NodeAttributes::setNodeClass(OpcUa_NodeClass* nodeClass) {
    if (d->hasAttachedValues) {
        delete d->nodeClass;
    }
    d->nodeClass = nodeClass;
}

Exception* NodeAttributes::getException() const {
    return d->exception;
}

void NodeAttributes::setException(CommonNamespace::Exception* exception) {
    if (d->hasAttachedValues) {
        delete d->exception;
    }
    d->exception = exception;
}
