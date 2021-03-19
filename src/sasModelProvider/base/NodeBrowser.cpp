#include <sasModelProvider/base/NodeBrowser.h>
#include <sasModelProvider/base/NodeBrowserException.h>
#include <opcuatypes.h> // ServiceContext
#include <opcua_types.h> // OpcUa_ViewDescription
#include <opcua_p_types.h> // OpcUa_UInt32
#include <continuationpoint.h> // BrowseContext
#include <opcua_builtintypes.h> // OpcUa_NodeId
#include <uaarraytemplates.h> // UaReferenceDescriptions
#include <map>
#include <sstream> // std::ostringstream
#include <vector>

namespace SASModelProviderNamespace {

    class NodeBrowserPrivate {
        friend class NodeBrowser;
    private:
        HaNodeManager* nodeManager;

        NodeManagerUaNode* getNodeManagerUaNode(const UaNodeId& nodeId);
    };

    NodeBrowser::NodeBrowser(HaNodeManager& nodeManager) {
        d = new NodeBrowserPrivate();
        d->nodeManager = &nodeManager;
    }

    NodeBrowser::~NodeBrowser() {
        delete d;
    }

    UaDataType* NodeBrowser::getDataType(const UaNodeId& nodeId) {
        UaNode* node = getNode(nodeId);
        return node != NULL && node->nodeClass() == OpcUa_NodeClass_DataType ?
                (UaDataType*) node : NULL;
    }

    UaObjectType* NodeBrowser::getObjectType(const UaNodeId& nodeId) {
        UaNode* node = getNode(nodeId);
        return node != NULL && node->nodeClass() == OpcUa_NodeClass_ObjectType ?
                (UaObjectType*) node : NULL;
    }

    UaObject* NodeBrowser::getObject(const UaNodeId& nodeId) {
        UaNode* node = getNode(nodeId);
        return node != NULL && node->nodeClass() == OpcUa_NodeClass_Object ?
                (UaObject*) node : NULL;
    }

    UaVariableType* NodeBrowser::getVariableType(const UaNodeId& nodeId) {
        UaNode* node = getNode(nodeId);
        return node != NULL && node->nodeClass() == OpcUa_NodeClass_VariableType ?
                (UaVariableType*) node : NULL;
    }

    UaVariable* NodeBrowser::getVariable(const UaNodeId& nodeId) {
        UaNode* node = getNode(nodeId);
        return node != NULL && node->nodeClass() == OpcUa_NodeClass_Variable ?
                (UaVariable*) node : NULL;
    }

    UaMethod* NodeBrowser::getMethod(const UaNodeId& nodeId) {
        UaNode* node = getNode(nodeId);
        return node != NULL && node->nodeClass() == OpcUa_NodeClass_Method ?
                (UaMethod*) node : NULL;
    }

    UaNode* NodeBrowser::getNode(const UaNodeId& nodeId) {
        NodeManagerUaNode* nmb = d->getNodeManagerUaNode(nodeId);
        return nmb == NULL ? NULL : nmb->getNode(nodeId);
    }

    UaStructureDefinition NodeBrowser::getStructureDefinition(const UaNodeId& dataTypeId) {
        NodeManagerUaNode* nmb = d->getNodeManagerUaNode(dataTypeId);
        return nmb == NULL ? UaStructureDefinition() : nmb->structureDefinition(dataTypeId);
    }

    std::vector<UaNodeId>* NodeBrowser::getSuperTypes(const UaNodeId& typeId)
    /* throws NodeBrowserException */ {
        std::vector<UaNodeId>* ret = new std::vector<UaNodeId>();
        ServiceContext sc;
        UaNodeId nodeToBrowse;
        UaNodeId referenceTypeId(OpcUaId_HasSubtype);
        BrowseContext bc(NULL /*view*/,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
                0 /*maxResultsToReturn*/,
                OpcUa_BrowseDirection_Inverse,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
                OpcUa_True /*includeSubtypes*/,
                0 /*nodeClassMask*/,
                OpcUa_BrowseResultMask_All /* resultMask */);
        UaNode* type = getNode(typeId);
        while (type != NULL && type->nodeId().namespaceIndex() != 0) {
            UaReferenceDescriptions referenceDescriptions;
            UaStatus result = type->browse(sc, bc, referenceDescriptions);
            if (!result.isGood()) {
                std::ostringstream msg;
                msg << "Cannot get super types for " << type->nodeId().toXmlString().toUtf8()
                        << ": " << result.toString().toUtf8();
                throw ExceptionDef(NodeBrowserException, msg.str());
            }
            UaNodeId* superTypeId = NULL;
            for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
                superTypeId = new UaNodeId(referenceDescriptions[i].NodeId.NodeId);
                ret->push_back(*superTypeId);
            }
            type->releaseReference();
            if (superTypeId == NULL) {
                type = NULL;
            } else {
                type = getNode(*superTypeId);
                delete superTypeId;
            }
        }
        if (type != NULL) {
            type->releaseReference();
        }
        return ret;
    }

    NodeManagerUaNode* NodeBrowserPrivate::getNodeManagerUaNode(const UaNodeId& nodeId) {
        if (0 == nodeId.namespaceIndex()) {
            return nodeManager->getNodeManagerRoot().getNodeManagerUaNode();
        }
        if (nodeManager->getNodeManagerBase().getNameSpaceIndex() == nodeId.namespaceIndex()) {
            return &nodeManager->getNodeManagerBase();
        }
        const std::vector<HaNodeManager*>* associatedNodeManagers
                = nodeManager->getAssociatedNodeManagers();
        if (associatedNodeManagers != NULL) {
            for (std::vector<HaNodeManager*>::const_iterator i = associatedNodeManagers->begin();
                    i != associatedNodeManagers->end(); i++) {
                HaNodeManager* nm = *i;
                if (nm->getNodeManagerBase().getNameSpaceIndex() == nodeId.namespaceIndex()) {
                    return &nm->getNodeManagerBase();
                }
            }
        }
        return NULL;
    }
} // namespace SASModelProviderNamespace
