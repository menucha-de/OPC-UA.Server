#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_OBJECTTYPEELEMENT_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_OBJECTTYPEELEMENT_H_

#include <uanodeid.h> // UaNodeId
#include <uaqualifiedname.h> // UaQualifiedName
#include <vector>

class ObjectTypeElementPrivate;

class ObjectTypeElement {
public:
    ObjectTypeElement(const UaNodeId& nodeId, OpcUa_NodeClass nodeClass,
            const UaNodeId& parent, const std::vector<const UaNodeId*>& childs,
            bool attachValues = false);
    virtual ~ObjectTypeElement();

    virtual const UaNodeId& getTypeId() const;
    virtual OpcUa_NodeClass getNodeClass() const;
    virtual const UaNodeId& getParent() const;
    virtual const std::vector<const UaNodeId*>& getChilds() const;
private:
    ObjectTypeElement(const ObjectTypeElement& event);
    ObjectTypeElement& operator=(const ObjectTypeElement& event);

    ObjectTypeElementPrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_OBJECTTYPEELEMENT_H_ */
