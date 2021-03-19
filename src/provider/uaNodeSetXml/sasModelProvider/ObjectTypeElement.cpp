#include "ObjectTypeElement.h"

class ObjectTypeElementPrivate {
    friend class ObjectTypeElement;
private:
    const UaNodeId* nodeId;
    OpcUa_NodeClass nodeClass;
    const UaNodeId* parent;
    const std::vector<const UaNodeId*>* childs;
    bool isRefEventType;
    bool hasAttachedValues;
};

ObjectTypeElement::ObjectTypeElement(const UaNodeId& nodeId, OpcUa_NodeClass nodeClass,
        const UaNodeId& parent, const std::vector<const UaNodeId*>& childs, bool attachValues) {
    d = new ObjectTypeElementPrivate();
    d->nodeId = &nodeId;
    d->nodeClass = nodeClass;
    d->parent = &parent;
    d->childs = &childs;
    d->hasAttachedValues = attachValues;
}

ObjectTypeElement::~ObjectTypeElement() {
    if (d->hasAttachedValues) {
        delete d->nodeId;
        delete d->parent;
        for (std::vector<const UaNodeId*>::const_iterator i = d->childs->begin();
                i != d->childs->end(); i++) {
            delete *i;
        }
        delete d->childs;
    }
    delete d;
}

const UaNodeId& ObjectTypeElement::getTypeId() const {
    return *d->nodeId;
}

OpcUa_NodeClass ObjectTypeElement::getNodeClass() const {
    return d->nodeClass;
}

const UaNodeId& ObjectTypeElement::getParent() const {
    return *d->parent;
}

const std::vector<const UaNodeId*>& ObjectTypeElement::getChilds() const {
    return *d->childs;
}