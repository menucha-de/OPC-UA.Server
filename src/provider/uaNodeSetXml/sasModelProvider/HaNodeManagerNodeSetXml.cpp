#include "HaNodeManagerNodeSetXml.h"
#include "HaNodeManagerNodeSetXmlException.h"
#include "HaXmlUaNodeFactoryNamespace.h"
#include "ObjectTypeElement.h"
#include <common/Exception.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <sasModelProvider/base/HaNodeManagerException.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <opcua_filetype.h> // OpcUa::FileType
#include <map>
#include <vector>
#include <sstream> // std::ostringstream
#include <nodemanagerroot.h>

using namespace CommonNamespace;

class HaNodeManagerNodeSetXmlPrivate {
    friend class HaNodeManagerNodeSetXml;
private:
    Logger* log;

    HaNodeManagerNodeSetXml* parent;
    const std::vector<HaNodeManager*>* associatedNodeManagers;
    EventTypeRegistry* eventTypeRegistry;
    HaXmlUaNodeFactoryManagerSet* xmlUaNodeFactoryManagerSet;
    IODataProviderNamespace::IODataProvider* ioDataProvider;
    HaNodeManagerIODataProviderBridge* nmioBridge;
    // xmlNodeId => objectTypeElement
    std::map<std::string, const ObjectTypeElement*> objectTypeElements;
    // xmlNodeId => nodeId
    std::map<std::string, UaNodeId> variables;

    // adds a variable or the variables of an object to internal list
    void addVariables(UaNode& node, const UaNodeId& parentNodeId, int indent)
    /* throws HaNodeManagerException */;
    // Adds object types and its components to internal list
    void addObjectTypeElements(UaNode& node, const UaNodeId& parentNodeId, int indent);
    bool isEventType(UaNode& node, OpcUa_BrowseDirection browseDirection)
    /* throws HaNodeManagerException */;
    // logs the nodes which are added to internal lists via addVariables or addObjectTypeElements
    void logNode(UaNode& node, const UaNodeId* parentNodeId, int indent);
};

HaNodeManagerNodeSetXml::HaNodeManagerNodeSetXml(const UaString& namespaceUri,
        const std::vector<HaNodeManager*>& associatedNodeManagers,
        EventTypeRegistry& eventTypeRegistry,
        HaXmlUaNodeFactoryManagerSet& uaNodeFactoryManagerSet,
        IODataProviderNamespace::IODataProvider& ioDataProvider)
: NodeManagerNodeSetXml(namespaceUri) {
    d = new HaNodeManagerNodeSetXmlPrivate();
    d->log = LoggerFactory::getLogger("HaNodeManagerNodeSetXml");
    d->parent = this;
    d->associatedNodeManagers = &associatedNodeManagers;
    d->eventTypeRegistry = &eventTypeRegistry;
    d->xmlUaNodeFactoryManagerSet = &uaNodeFactoryManagerSet;
    d->ioDataProvider = &ioDataProvider;
}

HaNodeManagerNodeSetXml::~HaNodeManagerNodeSetXml() {
    delete d;
}

void HaNodeManagerNodeSetXml::allNodesAndReferencesCreated() {
    HaNodeManagerException* exception = NULL;
    NodeBrowser nodeBrowser(*this);

    std::vector<UaObjectType*> objectTypes;
    // for each object type element
    for (std::map<std::string, const ObjectTypeElement*>::iterator i =
            d->objectTypeElements.begin(); i != d->objectTypeElements.end();
            i++) {
        const ObjectTypeElement& elem = *(*i).second;
        // if element is an object type (may also be a component like a variable)
        if (elem.getNodeClass() == OpcUa_NodeClass_ObjectType) {
            UaObjectType* objectType = nodeBrowser.getObjectType(elem.getTypeId());
            try {
                // if object type is an event type
                if (d->isEventType(*objectType, OpcUa_BrowseDirection_Both)) { // HaNodeManagerException                    
                    // add event type for registration at bridge
                    objectTypes.push_back(objectType);
                    // register event type
                    d->eventTypeRegistry->registerEventType(elem.getParent(),
                            elem.getTypeId());
                    const std::vector<const UaNodeId*>& childs = elem.getChilds();
                    // for each child
                    for (std::vector<const UaNodeId*>::const_iterator i =
                            childs.begin(); i != childs.end(); i++) {
                        const UaNodeId* childTypeId = *i;
                        UaVariable* variable = nodeBrowser.getVariable(*childTypeId);
                        // if child is a variable
                        if (variable != NULL) {
                            // register variable as event field
                            d->eventTypeRegistry->registerEventField(elem.getTypeId(),
                                    *childTypeId, variable->browseName());
                            variable->releaseReference();
                        }
                    }
                } else {
                    objectType->releaseReference();
                }
            } catch (Exception& e) {
                objectType->releaseReference();
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerException,
                            std::string("Cannot register event type: ")
                            .append(objectType->nodeId().toXmlString().toUtf8()));
                    exception->setCause(&e);
                }
            }
        }
    }
    try {
        d->nmioBridge->updateValueHandling(objectTypes); // HaNodeManagerIODataProviderBridgeException
    } catch (Exception& e) {
        if (exception == NULL) {
            exception = new ExceptionDef(HaNodeManagerException,
                    std::string("Cannot update value handling for event types"));
            exception->setCause(&e);
        }
    }
    // release references to object type nodes
    for (int i = 0; i < objectTypes.size(); i++) {
        objectTypes[i]->releaseReference();
    }

    std::vector<UaVariable*> variables;
    // for each variable
    for (std::map<std::string, UaNodeId>::iterator i =
            d->variables.begin(); i != d->variables.end(); i++) {
        const std::string& nodeIdXml = i->first;
        const UaNodeId& nodeId = i->second;
        // if variable is NOT part of an object type definition
        if (d->objectTypeElements.find(nodeIdXml) == d->objectTypeElements.end()) {
            // add variable for registration at bridge
            variables.push_back(nodeBrowser.getVariable(nodeId));
        }
    }
    try {
        d->nmioBridge->updateValueHandling(variables); // HaNodeManagerIODataProviderBridgeException
    } catch (Exception& e) {
        if (exception == NULL) {
            exception = new ExceptionDef(HaNodeManagerException,
                    std::string("Cannot update value handling for variables"));
            exception->setCause(&e);
        }
    }
    // release references to variable nodes
    for (int i = 0; i < variables.size(); i++) {
        variables[i]->releaseReference();
    }

    // clear lists
    for (std::map<std::string, const ObjectTypeElement*>::iterator i =
            d->objectTypeElements.begin(); i != d->objectTypeElements.end(); i++) {
        delete (*i).second;
    }
    d->objectTypeElements.clear();
    d->variables.clear();

    if (exception != NULL) {
        std::ostringstream msg;
        msg << "Creating node set for namespace index " << getNameSpaceIndex() << " failed";
        HaNodeManagerNodeSetXmlException ex = ExceptionDef(HaNodeManagerNodeSetXmlException,
                msg.str());
        ex.setCause(exception);
        delete exception;
        // there is no way to inform the OPC UA server => log the exception
        std::string st;
        ex.getStackTrace(st);
        d->log->error("Exception while reading node set: %s", st.c_str());
    } else {
        d->log->info("Created node set for namespace index %d", getNameSpaceIndex());
    }
}

void HaNodeManagerNodeSetXml::variableTypeCreated(UaVariableType* pNewNode,
        UaBase::VariableType * pVariableType) {
}

void HaNodeManagerNodeSetXml::variableCreated(UaVariable* pNewNode,
        UaBase::Variable * pVariable) {
    d->addVariables(*pNewNode, pVariable->parentNodeId(), 0 /* indent */); // HaNodeManagerException
}

void HaNodeManagerNodeSetXml::objectTypeCreated(UaObjectType* pNewNode,
        UaBase::ObjectType * pObjectType) {
    d->addObjectTypeElements(*pNewNode, pObjectType->superTypeId(), 0 /*indent*/);
}

void HaNodeManagerNodeSetXml::objectCreated(UaObject* pNewNode,
        UaBase::Object * pObject) {
    d->addVariables(*pNewNode, pObject->parentNodeId(), 0 /* indent */); // HaNodeManagerException
}

void HaNodeManagerNodeSetXml::methodCreated(UaMethod* pNewNode,
        UaBase::Method * pMethod) {
}

void HaNodeManagerNodeSetXml::dataTypeCreated(UaDataType* pNewNode, UaBase::DataType * pDataType) {
    //    UaStructureDefinition sd = structureDefinition(pNewNode->nodeId());
    //    fprintf(stderr, "struct name=%s,dataTypeId=%s,hasOptionalFields=%d\n",
    //            sd.name().toUtf8(), sd.dataTypeId().toXmlString().toUtf8(), sd.hasOptionalFields());
    //    for (int j = 0; j < sd.childrenCount(); j++) {
    //        UaStructureField sf = sd.child(j);
    //        fprintf(stderr, "  field name=%s,dataTypeId=%s,isOptional=%d,arrayType=%d\n",
    //                sf.name().toUtf8(), sf.typeId().toXmlString().toUtf8(), sf.isOptional(),
    //                sf.arrayType());
    //    }
}

UaStatus HaNodeManagerNodeSetXml::afterStartUp() {
    UaStatus ret = NodeManagerNodeSetXml::afterStartUp();
    if (ret.isNotGood()) {
        return ret;
    }
    // add a namespace related UaNode factory to the UaNode factory managers
    // (this node manager is added as method manager to all created objects)
    d->xmlUaNodeFactoryManagerSet->addNamespace(
            *new HaXmlUaNodeFactoryNamespace(getNameSpaceIndex(), *this /*methodManager*/));
    // create the bridge to the IO data provider
    d->nmioBridge = new HaNodeManagerIODataProviderBridge(*this,
            *d->ioDataProvider);
    ret = d->nmioBridge->afterStartUp();
    if (ret.isNotGood()) {
        return ret;
    }
    if (d->log->isInfoEnabled()) {
        d->log->info("Started node manager for namespace %s with index %d",
                getNameSpaceUri().toUtf8(), getNameSpaceIndex());
    }
    return ret;
}

UaStatus HaNodeManagerNodeSetXml::beforeShutDown() {
    UaStatus ret1 = d->nmioBridge->beforeShutDown();
    delete d->nmioBridge;
    UaStatus ret2 = NodeManagerNodeSetXml::beforeShutDown();
    return ret1.isNotGood() ? ret1 : ret2;
}

UaStatus HaNodeManagerNodeSetXml::readValues(const UaVariableArray& arrUaVariables,
        UaDataValueArray & arrDataValues) {
    return d->nmioBridge->readValues(arrUaVariables, arrDataValues);
}

UaStatus HaNodeManagerNodeSetXml::writeValues(const UaVariableArray& arrUaVariables,
        const PDataValueArray& arrpDataValues,
        UaStatusCodeArray & arrStatusCodes) {
    return d->nmioBridge->writeValues(arrUaVariables, arrpDataValues,
            arrStatusCodes);
}

OpcUa_Boolean HaNodeManagerNodeSetXml::beforeSetAttributeValue(
        Session* pSession, UaNode* pNode, OpcUa_Int32 attributeId,
        const UaDataValue& dataValue, OpcUa_Boolean& checkWriteMask) {
	return d->nmioBridge->beforeSetAttributeValue(pSession, pNode, attributeId, dataValue, checkWriteMask);
}

void HaNodeManagerNodeSetXml::afterSetAttributeValue(Session* pSession, UaNode* pNode,
        OpcUa_Int32 attributeId, const UaDataValue & dataValue) {
    d->nmioBridge->afterSetAttributeValue(pSession, pNode, attributeId,
            dataValue);
}

void HaNodeManagerNodeSetXml::variableCacheMonitoringChanged(UaVariableCache* pVariable,
        TransactionType transactionType) {
    d->nmioBridge->variableCacheMonitoringChanged(pVariable, transactionType);
}

UaStatus HaNodeManagerNodeSetXml::beginCall(MethodManagerCallback* pCallback,
        const ServiceContext& serviceContext, OpcUa_UInt32 callbackHandle,
        MethodHandle* pMethodHandle, const UaVariantArray & inputArguments) {
    return d->nmioBridge->beginCall(pCallback, serviceContext, callbackHandle,
            pMethodHandle, inputArguments);
}

NodeManager & HaNodeManagerNodeSetXml::getNodeManagerRoot() {
    return *m_pServerManager->getNodeManagerRoot();
}

NodeManagerBase & HaNodeManagerNodeSetXml::getNodeManagerBase() {
    return *this;
}

const std::vector<HaNodeManager*>* HaNodeManagerNodeSetXml::getAssociatedNodeManagers() {
    return d->associatedNodeManagers;
}

EventTypeRegistry & HaNodeManagerNodeSetXml::getEventTypeRegistry() {
    return *d->eventTypeRegistry;
}

HaNodeManagerIODataProviderBridge & HaNodeManagerNodeSetXml::getIODataProviderBridge() {
    return *d->nmioBridge;
}

UaString HaNodeManagerNodeSetXml::getNameSpaceUri() {
    return NodeManagerNodeSetXml::getNameSpaceUri();
}

const UaString & HaNodeManagerNodeSetXml::getDefaultLocaleId() const {
    return m_defaultLocaleId;
}

void HaNodeManagerNodeSetXml::setVariable(UaVariable & variable,
        UaVariant & newValue) /* throws HaNodeManagerException */ {
    const OpcUa_Variant* cacheValue = variable.value(
            NULL /* session */).value();
    if (d->log->isInfoEnabled()) {
        d->log->info("SET %-20s nodeId=%s,oldValue=%s,newValue=%s",
                variable.browseName().toString().toUtf8(),
                variable.nodeId().toXmlString().toUtf8(),
                UaVariant(*cacheValue).toString().toUtf8(),
                newValue.toFullString().toUtf8());
    }
    // set new value
    UaDataValue dataValue;
    dataValue.setValue(newValue, OpcUa_False /* detachValue */, OpcUa_True /* updateTimeStamps */);
    UaStatus status = variable.setValue(NULL /* session */, dataValue,
            OpcUa_False /* checkAccessLevel */);
    if (!status.isGood()) {
        std::ostringstream msg;
        msg << "Cannot set value to variable: " << status.toString().toUtf8()
                << " nodeId=" << variable.nodeId().toXmlString().toUtf8()
                << ",dataType=" << variable.dataType().toXmlString().toUtf8()
                << ",variantType=" << newValue.dataType().toXmlString().toUtf8()
                << ",oldValue=" << UaVariant(*cacheValue).toString().toUtf8()
                << ",newValue=" << newValue.toFullString().toUtf8();
        throw ExceptionDef(HaNodeManagerException, msg.str());
    }
}

void HaNodeManagerNodeSetXmlPrivate::addVariables(UaNode& node, const UaNodeId& parentNodeId,
        int indent)/* throws HaNodeManagerException */ {
    std::string nodeIdXml(node.nodeId().toXmlString().toUtf8());
    if (variables.find(nodeIdXml) != variables.end()) {
        return;
    }

    //    if (node.typeDefinitionId() == UaNodeId(OpcUaId_FileType)) {
    //        OpcUa::FileType& ft = static_cast<OpcUa::FileType&> (node);
    //        ft.setFilePath("/home/markus/test/data.txt");
    //        ft.setWritable(true);
    //        ft.setUserWritable(true);
    //
    //        ServiceContext serviceContext;
    //        ServerManager* pServerManager = NodeManagerRoot::CreateRootNodeManager()->pServerManager();
    //        serviceContext.setSession(pServerManager->getInternalSession());
    //        // Session* pInternalSession = pServerManager->createInternalSession("InternalSession", "en", NULL);
    //        // pInternalSession->releaseReference();
    //        OpcUa_UInt32 fileHandleWrite;
    //        UaStatus status = ft.Open(serviceContext, 2, fileHandleWrite);
    //        if (!status.isGood()) {
    //            fprintf(stderr, "### Cannot open for writing: %s\n", status.toString().toUtf8());
    //        } else {
    //            ft.Close(serviceContext, fileHandleWrite);
    //        }
    //        OpcUa_UInt32 fileHandle;
    //        status = ft.Open(serviceContext, 1, fileHandle);
    //        if (!status.isGood()) {
    //            fprintf(stderr, "### Cannot open for reading: %s\n", status.toString().toUtf8());
    //        } else {
    //            UaByteString data;
    //            ft.Read(serviceContext, fileHandle, ft.getSize(), data);
    //            fprintf(stderr, "############ %s\n", UaString(data).toUtf8());
    //            ft.Close(serviceContext, fileHandle);
    //        }
    //        return;
    //    }

    switch (node.nodeClass()) {
        case OpcUa_NodeClass_Variable:
        {
            logNode(node, &parentNodeId, indent);
            // add variable to list
            variables[nodeIdXml] = node.nodeId();
            break;
        }
        case OpcUa_NodeClass_Object:
            break;
    }

    ServiceContext sc;
    UaNodeId nodeToBrowse;
    UaNodeId referenceTypeId(OpcUaId_HierarchicalReferences);
    BrowseContext bc(NULL /*view*/,
            (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
            0 /*maxResultsToReturn*/,
            OpcUa_BrowseDirection_Forward,
            (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
            OpcUa_True /*includeSubtypes*/,
            0 /*nodeClassMask*/,
            OpcUa_BrowseResultMask_All /* resultMask */);
    UaReferenceDescriptions referenceDescriptions;
    UaStatus result = node.browse(sc, bc, referenceDescriptions);
    if (!result.isGood()) {
        std::ostringstream msg;
        msg << "Cannot browse for variables starting with " << node.nodeId().toXmlString().toUtf8()
                << ": " << result.toString().toUtf8();
        throw ExceptionDef(HaNodeManagerException, msg.str());
    }
    if (referenceDescriptions.length() > 0) {
        indent++;
        NodeBrowser nodeBrowser(*parent);
        // for each reference
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId referencedNodeId(referenceDescriptions[i].NodeId.NodeId);
            // get referenced node
            UaNode* referencedNode = nodeBrowser.getNode(referencedNodeId);
            if (referencedNode != NULL) {
                switch (referencedNode->nodeClass()) {
                    case OpcUa_NodeClass_Variable:
                    case OpcUa_NodeClass_Object:
                        // add referenced node
                        addVariables(*referencedNode, node.nodeId() /*parent*/, indent);
                        break;
                }
                referencedNode->releaseReference();
            } else {
                std::ostringstream msg;
                msg << "Cannot find referenced node " << referencedNodeId.toXmlString().toUtf8()
                        << " from " << node.nodeId().toXmlString().toUtf8();
                throw ExceptionDef(HaNodeManagerException, msg.str());
            }
        }
    }
}

void HaNodeManagerNodeSetXmlPrivate::addObjectTypeElements(UaNode& node, const UaNodeId& parentNodeId,
        int indent) {
    std::string nodeIdXml(node.nodeId().toXmlString().toUtf8());
    if (objectTypeElements.find(nodeIdXml) != objectTypeElements.end()) {
        return;
    }

    logNode(node, &parentNodeId, indent);
    std::vector<const UaNodeId*>* elemChilds = new std::vector<const UaNodeId*>();
    // add object type or a component to list                
    objectTypeElements[nodeIdXml] = new ObjectTypeElement(*new UaNodeId(node.nodeId()),
            node.nodeClass(), *new UaNodeId(parentNodeId), *elemChilds, true /* attachValues */);

    ServiceContext sc;
    UaNodeId nodeToBrowse;
    UaNodeId referenceTypeId(OpcUaId_HierarchicalReferences);
    BrowseContext bc(NULL /*view*/,
            (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
            0 /*maxResultsToReturn*/,
            OpcUa_BrowseDirection_Both,
            (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
            OpcUa_True /*includeSubtypes*/,
            0 /*nodeClassMask*/,
            OpcUa_BrowseResultMask_All /* resultMask */);
    UaReferenceDescriptions referenceDescriptions;
    UaStatus result = node.browse(sc, bc, referenceDescriptions);
    if (!result.isGood()) {
        std::ostringstream msg;
        msg << "Cannot browse for object type starting with "
                << node.nodeId().toXmlString().toUtf8()
                << ": " << result.toString().toUtf8();
        throw ExceptionDef(HaNodeManagerException, msg.str());
    }
    if (referenceDescriptions.length() > 0) {
        indent++;
        NodeBrowser nodeBrowser(*this->parent);
        // for each reference
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId referencedNodeId(referenceDescriptions[i].NodeId.NodeId);
            // if reference to child node
            if (referenceDescriptions[i].IsForward) {
                // get referenced node
                UaNode* referencedNode = nodeBrowser.getNode(referencedNodeId);
                if (referencedNode != NULL) {
                    // add referenced node to child list
                    elemChilds->push_back(new UaNodeId(referencedNodeId));
                    // add referenced node                
                    addObjectTypeElements(*referencedNode, node.nodeId() /*parent*/, indent);
                    referencedNode->releaseReference();
                } else {
                    std::ostringstream msg;
                    msg << "Cannot find referenced node " << referencedNodeId.toXmlString().toUtf8()
                            << " from " << node.nodeId().toXmlString().toUtf8();
                    throw ExceptionDef(HaNodeManagerException, msg.str());
                }
            } else {
                // reference to parent of an object type is not available here
                // => parameter value must be used
            }
        }
    }
}

bool HaNodeManagerNodeSetXmlPrivate::isEventType(UaNode& node,
        OpcUa_BrowseDirection browseDirection) /* throws HaNodeManagerException */ {
    ServiceContext sc;
    UaNodeId nodeToBrowse;
    UaNodeId referenceTypeId(OpcUaId_References);
    BrowseContext bc(NULL /*view*/,
            (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
            0 /*maxResultsToReturn*/,
            OpcUa_BrowseDirection_Both,
            (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
            OpcUa_True /*includeSubtypes*/,
            OpcUa_NodeClass_ObjectType /*nodeClassMask*/,
            OpcUa_BrowseResultMask_All /* resultMask */);
    UaReferenceDescriptions referenceDescriptions;
    UaStatus result = node.browse(sc, bc, referenceDescriptions);
    if (!result.isGood()) {
        std::ostringstream msg;
        msg << "Cannot browse for GenerateEvent references starting with "
                << node.nodeId().toXmlString().toUtf8() << ": " << result.toString().toUtf8();
        throw ExceptionDef(HaNodeManagerException, msg.str());
    }
    if (referenceDescriptions.length() > 0) {
        NodeBrowser nodeBrowser(*parent);
        // for each reference
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId referencedNodeId(referenceDescriptions[i].NodeId.NodeId);
            OpcUa_Boolean isForward = referenceDescriptions[i].IsForward;
            switch (UaNodeId(referenceDescriptions[i].ReferenceTypeId).identifierNumeric()) {
                case OpcUaId_GeneratesEvent:
                    if (!isForward) {
                        return true;
                    }
                    break;
                case OpcUaId_HasSubtype:
                    if (browseDirection == OpcUa_BrowseDirection_Both
                            || browseDirection == OpcUa_BrowseDirection_Forward && isForward
                            || browseDirection == OpcUa_BrowseDirection_Inverse && !isForward) {
                        // get referenced node
                        UaNode* referencedNode = nodeBrowser.getNode(referencedNodeId);
                        if (referencedNode != NULL) {
                            try {
                                // if referenced node is event type
                                if (isEventType(*referencedNode, isForward
                                        ? OpcUa_BrowseDirection_Forward
                                        : OpcUa_BrowseDirection_Inverse)) { // HaNodeManagerException
                                    referencedNode->releaseReference();
                                    return true;
                                }
                                referencedNode->releaseReference();
                            } catch (Exception& e) {
                                referencedNode->releaseReference();
                                throw;
                            }
                        } else {
                            std::ostringstream msg;
                            msg << "Cannot find referenced node "
                                    << referencedNodeId.toXmlString().toUtf8()
                                    << " from " << node.nodeId().toXmlString().toUtf8();
                            throw ExceptionDef(HaNodeManagerException, msg.str());
                        }
                    }
                    break;
            }
        }
    }
    return false;
}

void HaNodeManagerNodeSetXmlPrivate::logNode(UaNode& node, const UaNodeId* parentNodeId,
        int indent) {
    std::ostringstream msg;
    for (OpcUa_Int16 i = 0; i < indent; i++) {
        msg << " ";
    }
    switch (node.nodeClass()) {
        case OpcUa_NodeClass_Variable:
            msg << "VAR ";
            break;
        case OpcUa_NodeClass_Object:
            msg << "OBJ ";
            break;
        case OpcUa_NodeClass_Method:
            msg << "MET ";
            break;
        case OpcUa_NodeClass_ObjectType:
            msg << "OBJT ";
            break;
    }
    msg << node.browseName().toString().toUtf8();
    for (OpcUa_Int16 i = 0; i < 20 - indent
            - UaString(node.browseName().name()).length(); i++) {
        msg << " ";
    }
    msg << " nodeId=" << node.nodeId().toXmlString().toUtf8();
    if (parentNodeId != NULL) {
        msg << ",parentNodeId=" << parentNodeId->toXmlString().toUtf8();
    }
    switch (node.nodeClass()) {
        case OpcUa_NodeClass_Variable:
        {
            UaVariable& variable = static_cast<UaVariable&> (node);
            msg << ",typeDefinitionId=" << node.typeDefinitionId().toXmlString().toUtf8();
            msg << ",dataType=" << variable.dataType().toXmlString().toUtf8();
            break;
        }
        case OpcUa_NodeClass_Object:
            msg << ",typeDefinitionId=" << node.typeDefinitionId().toXmlString().toUtf8();
            break;
    }
    log->debug("%s", msg.str().c_str());
}
