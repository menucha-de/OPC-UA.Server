#ifndef SASMODELPROVIDER_BASE_NODEBROWSER_H_
#define SASMODELPROVIDER_BASE_NODEBROWSER_H_

#include <sasModelProvider/base/HaNodeManager.h>
#include <uabasenodes.h> // UaObjectType
#include <uanodeid.h> // UaNodeId
#include <uastructuredefinition.h> // UaStructureDefinition
#include <vector>

namespace SASModelProviderNamespace {

    class NodeBrowserPrivate;

    class NodeBrowser {
    public:
        NodeBrowser(HaNodeManager& nodeManager);
        virtual ~NodeBrowser();

        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaDataType* getDataType(const UaNodeId& nodeId);
        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaObjectType* getObjectType(const UaNodeId& nodeId);
        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaObject* getObject(const UaNodeId& nodeId);
        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaVariableType* getVariableType(const UaNodeId& nodeId);
        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaVariable* getVariable(const UaNodeId& nodeId);
        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaMethod* getMethod(const UaNodeId& nodeId);
        // If an object is found for the nodeId, the reference count of the object is 
        // incremented. The caller must release the reference with method "releaseReference"
        // when the object is no longer needed.
        virtual UaNode* getNode(const UaNodeId& nodeId);

        virtual UaStructureDefinition getStructureDefinition(const UaNodeId& dataTypeId);
        // Returns the super types incl. the first one in namespace 0 starting with the nearest parent.
        virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& typeId)
        /* throws NodeBrowserException */;

    private:
        NodeBrowserPrivate* d;
    };

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_NODEBROWSER_H_ */
