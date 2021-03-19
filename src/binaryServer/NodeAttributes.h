#ifndef BINARYSERVER_NODEATTRIBUTES_H
#define BINARYSERVER_NODEATTRIBUTES_H

#include <common/Exception.h>
#include <uanodeid.h>
#include <uavariant.h>

class NodeAttributesPrivate;

class NodeAttributes {
public:
    NodeAttributes(UaNodeId& nodeId, bool attachValues = false);
    // Creates a deep copy of the instance.
    NodeAttributes(const NodeAttributes& orig);
    virtual ~NodeAttributes();

    virtual UaNodeId& getNodeId() const;
    virtual UaVariant* getValue() const;
    virtual void setValue(UaVariant* value);
    virtual UaNodeId* getDataType() const;
    virtual void setDataType(UaNodeId* dataType);
    virtual OpcUa_NodeClass* getNodeClass() const;
    virtual void setNodeClass(OpcUa_NodeClass* nodeClass);
    
    virtual CommonNamespace::Exception* getException() const;
    virtual void setException(CommonNamespace::Exception* exception);
private:
    NodeAttributes& operator=(const NodeAttributes&);

    NodeAttributesPrivate* d;
};

#endif /* BINARYSERVER_NODEATTRIBUTES_H */

