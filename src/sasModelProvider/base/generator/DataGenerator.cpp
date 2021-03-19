#include "DataGenerator.h"
#include "GeneratorException.h"
#include <common/Exception.h> // ExceptionDef
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/OpcUaEventData.h>
#include <ioDataProvider/Scalar.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <uaargument.h> // UaArgument
#include "uagenericstructurevalue.h" // UaGenericStructureValue
#include "uagenericunionvalue.h" // UaGenericUnionValue
#include <uanodeid.h> // UaNodeId
#include <nodemanagerroot.h> // NodeManagerRoot
#include <servermanager.h> // ServerManager
#include <session.h> // Session
#include <algorithm> // std::find
#include <sstream> // std::ostringstream
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;
using namespace IODataProviderNamespace;

namespace SASModelProviderNamespace {

    class DataGeneratorPrivate {
        friend class DataGenerator;
    private:
        Logger* log;
        NodeBrowser* nodeBrowser;
        ConverterUa2IO* converter;
        OpcUa_Int16 valueCounter;
        ServerManager* serverManager;
        Session* internalSession;

        UaNodeId getBuildInType(const UaNodeId& dataTypeId) /* throws GeneratorException */;

        void generateEventFieldData(UaObjectType& rootObjectType, UaObjectType& objectType,
                std::vector<UaQualifiedName>& returnBrowseNames,
                std::vector<const NodeData*>& returnEventFieldData) /* throws ConversionException, GeneratorExceptionn */;
        UaNodeId* getEventSourceNode(UaObjectType& objectType) /* throws GeneratorExceptionn */;
        UaNodeId* getInstance(UaNode& root, std::vector<UaNodeId*>& type) /* throws GeneratorExceptionn */;
        void getSubTypes(UaNode& type, std::vector<UaNodeId*>& returnSubTypes) /* throws GeneratorExceptionn */;

        UaVariant * generate(const UaNodeId& dataTypeId, OpcUa_Boolean isArray,
                OpcUa_Int16 indent) /* throws ConversionException, GeneratorException */;
        UaVariant * generateBuildInType(const UaNodeId& dataTypeId,
                OpcUa_Int16 indent) /* throws GeneratorException */;
        UaVariant * generateStructure(const UaNodeId& dataTypeId,
                OpcUa_Int16 indent) /* throws ConversionException, GeneratorExceptionn */;
        UaVariant * generateStructureField(UaStructureField& field,
                OpcUa_Int16 indent) /* throws ConversionException, GeneratorExceptionn */;
        UaVariant * generateArray(const UaNodeId& dataTypeId,
                OpcUa_Int16 indent) /* throws ConversionException, GeneratorExceptionn */;
        UaVariant * convert2ExtensionObjectArray(std::vector<UaVariant*>& values);
        UaVariant * convert2Int32Array(std::vector<UaVariant*>& values);
        UaVariant * convert2UInt32Array(std::vector<UaVariant*>& values);
        UaVariant * convert2LocalizedTextArray(std::vector<UaVariant*>& values);
    };

    DataGenerator::DataGenerator(NodeBrowser& nodeBrowser, ConverterUa2IO& converter) {
        d = new DataGeneratorPrivate();
        d->log = LoggerFactory::getLogger("DataGenerator");
        d->nodeBrowser = &nodeBrowser;
        d->converter = &converter;
        d->valueCounter = 1;
        d->serverManager = NodeManagerRoot::CreateRootNodeManager()->pServerManager();
        d->internalSession = d->serverManager->createInternalSession("InternalSession", "en",
                NULL /*userIdentityToken*/);
    }

    DataGenerator::~DataGenerator() {
        d->internalSession->releaseReference();
        delete d;
    }

    std::vector<UaVariant*>* DataGenerator::generate(
            const std::vector<UaVariable*>& variables) /* throws ConversionException, GeneratorExceptionn */ {
        std::vector<UaVariant*>* ret = new std::vector<UaVariant*>();
        for (int i = 0; i < variables.size(); i++) {
            UaVariable& variable = *variables[i];                        
            if (variable.typeDefinitionId() != UaNodeId(OpcUaId_DataTypeDictionaryType) &&
                    variable.typeDefinitionId() != UaNodeId(OpcUaId_DataTypeDescriptionType)) {
                OpcUa_Boolean isArray = variable.valueRank() == 1;
                try {                    
                    UaVariant* value = d->generate(variable.dataType(), isArray,
                            0 /*indent*/); // ConversionException, GeneratorExceptionn
                    if (d->log->isInfoEnabled()) {
                        // convert UaVariant to Variant
                        Variant* v = d->converter->convertUa2io(*value, variable.dataType()); // ConversionException
                        ScopeGuard<Variant> vSG(v);
                        d->log->info("VAR %-20s nodeId=%s,typeDefinitionId=%s,dataType=%s,isArray=%d,value=%s",
                                variable.browseName().toString().toUtf8(),
                                variable.nodeId().toXmlString().toUtf8(),
                                variable.typeDefinitionId().toXmlString().toUtf8(),
                                variable.dataType().toXmlString().toUtf8(), isArray,
                                v->toString().c_str());
                    }
                    ret->push_back(value);
                } catch (Exception& e) {
                    ret->push_back(NULL);
                    // process as many variables as possible
                    std::string st;
                    e.getStackTrace(st);
                    d->log->error("Exception while generating variable data: %s", st.c_str());
                }
            }
        }
        return ret;
    }

    std::vector<UaVariant*>* DataGenerator::generate(
            const std::vector<UaArgument*>& arguments) /* throws ConversionException, GeneratorExceptionn */ {
        std::vector<UaVariant*>* ret = new std::vector<UaVariant*>();
        for (std::vector<UaArgument*>::const_iterator i = arguments.begin();
                i != arguments.end(); i++) {
            UaArgument& arg = **i;
            OpcUa_Boolean isArray = arg.getValueRank() == 1;
            try {
                UaVariant* value = d->generate(arg.getDataType(), isArray,
                        0 /*indent*/); // ConversionException, GeneratorExceptionn            
                if (d->log->isInfoEnabled()) {
                    // convert UaVariant to Variant
                    Variant* v = d->converter->convertUa2io(*value, arg.getDataType()); // ConversionException
                    ScopeGuard<Variant> vSG(v);
                    d->log->info("MARG %-20s dataType=%s,isArray=%d,value=%s",
                            arg.getName().toUtf8(), arg.getDataType().toXmlString().toUtf8(),
                            isArray, v->toString().c_str());
                }
                ret->push_back(value);
            } catch (Exception& e) {
                ret->push_back(NULL);
                // process as many arguments as possible
                std::string st;
                e.getStackTrace(st);
                d->log->error("Exception while generating argument data: %s", st.c_str());
            }
        }
        return ret;
    }

    std::vector<OpcUaEventData*>* DataGenerator::generate(
            const std::vector<UaObjectType*>& objectTypes) /* throws ConversionException, GeneratorExceptionn */ {
        std::vector<OpcUaEventData*>* ret = new std::vector<OpcUaEventData*>();
        for (int i = 0; i < objectTypes.size(); i++) {
            UaObjectType& objectType = *objectTypes[i];
            try {
                std::vector<const NodeData*>* generatedData = new std::vector<const NodeData*>();
                VectorScopeGuard<const NodeData> generatedDataSG(generatedData);
                std::vector<UaQualifiedName> browseNames;
                d->generateEventFieldData(objectType, objectType, browseNames, *generatedData); // ConversionException, GeneratorExceptionn            
                std::vector<const NodeData*> generatedDataCopy(*generatedData);
                // get data for default fields
                const NodeId* sourceNodeId = NULL;
                const std::string* message = NULL;
                int severity = 0;
                bool severityFound = false;
                for (int j = 0; j < generatedDataCopy.size(); j++) {
                    const NodeData* data = generatedDataCopy[j];
                    if (data == NULL) {
                        continue;
                    }
                    if (0 == data->getNodeId().getNamespaceIndex()) {
                        switch (data->getNodeId().getNumeric()) {
                            case OpcUaId_BaseEventType_SourceNode:
                            {
                                const NodeId* v = static_cast<const NodeId*> (data->getData());
                                sourceNodeId = new NodeId(*v);
                                break;
                            }
                            case OpcUaId_BaseEventType_Message:
                            {
                                const Scalar* v = static_cast<const Scalar*> (data->getData());
                                message = new std::string(*v->getString());
                                break;
                            }
                            case OpcUaId_BaseEventType_Severity:
                            {
                                const Scalar* v = static_cast<const Scalar*> (data->getData());
                                severity = v->getInt();
                                severityFound = true;
                                break;
                            }
                        }
                        // remove data from event fields
                        std::vector<const NodeData*>::iterator it =
                                std::find(generatedData->begin(), generatedData->end(), data);
                        delete *it;
                        generatedData->erase(it);
                    }
                }
                if (sourceNodeId == NULL) {
                    throw ExceptionDef(GeneratorException,
                            std::string("Missing default event field 'SourceNode' for event type ")
                            .append(objectType.nodeId().toXmlString().toUtf8()));
                }
                if (message == NULL) {
                    throw ExceptionDef(GeneratorException,
                            std::string("Missing default event field 'Message' for event type ")
                            .append(objectType.nodeId().toXmlString().toUtf8()));
                }
                if (!severityFound) {
                    throw ExceptionDef(GeneratorException,
                            std::string("Missing default event field 'Severity' for event type ")
                            .append(objectType.nodeId().toXmlString().toUtf8()));
                }
                OpcUaEventData* eventData = new OpcUaEventData(*sourceNodeId, *message, severity,
                        *generatedDataSG.detach(), true /*attachValues*/);
                if (d->log->isInfoEnabled()) {
                    d->log->info("OBJT %-20s nodeId=%s,value=%s",
                            objectType.browseName().toString().toUtf8(),
                            objectType.nodeId().toXmlString().toUtf8(),
                            eventData->toString().c_str());
                }
                // add OPC UA event data to list
                ret->push_back(eventData);
            } catch (Exception& e) {
                ret->push_back(NULL);
                GeneratorException ex = ExceptionDef(GeneratorException,
                        std::string("Cannot create data for event type ")
                        .append(objectType.nodeId().toXmlString().toUtf8()));
                ex.setCause(&e);
                std::string st;
                ex.getStackTrace(st);
                d->log->error("Exception while generating object type data: %s", st.c_str());
            }
        }
        return ret;
    }

    UaNodeId DataGeneratorPrivate::getBuildInType(
            const UaNodeId& dataTypeId) /* throws GeneratorException */ {
        if (0 == dataTypeId.namespaceIndex()) {
            return dataTypeId;
        }
        std::vector<UaNodeId>* superTypes = nodeBrowser->getSuperTypes(dataTypeId);
        if (superTypes != NULL) {
            ScopeGuard<std::vector<UaNodeId> > superTypesSG(superTypes);
            if (superTypes->size() > 0) {
                return superTypes->back();
            }
        }
        std::ostringstream msg;
        msg << "Cannot get base type of " << dataTypeId.toXmlString().toUtf8();
        throw ExceptionDef(GeneratorException, msg.str());
    }

    void DataGeneratorPrivate::generateEventFieldData(UaObjectType& rootObjectType,
            UaObjectType& objectType, std::vector<UaQualifiedName>& returnBrowseNames,
            std::vector<const NodeData*>& returnEventFieldData)
    /* throws ConversionException, GeneratorExceptionn */ {
        ServiceContext sc;
        UaNodeId nodeToBrowse;
        UaNodeId referenceTypeId(OpcUaId_HierarchicalReferences);
        BrowseContext bc(NULL /*view*/,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
                0 /*maxResultsToReturn*/,
                OpcUa_BrowseDirection_Both,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
                OpcUa_True /*includeSubtypes*/,
                OpcUa_NodeClass_ObjectType // super types
                | OpcUa_NodeClass_Variable /*nodeClassMask*/,
                OpcUa_BrowseResultMask_All /*resultMask*/);

        UaReferenceDescriptions referenceDescriptions;
        UaStatus result = objectType.browse(sc, bc, referenceDescriptions);
        if (!result.isGood()) {
            std::ostringstream msg;
            msg << "Cannot get event fields for " << objectType.nodeId().toXmlString().toUtf8()
                    << ": " << result.toString().toUtf8();
            throw ExceptionDef(GeneratorException, msg.str());
        }
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
            UaVariable* variable = NULL;
            try {
                // if variable
                if (OpcUaId_HasSubtype
                        != UaNodeId(referenceDescriptions[i].ReferenceTypeId).identifierNumeric()) {
                    // check for duplicate browse name (eg. overridden variable)
                    bool found = false;
                    for (int j = 0; j < returnBrowseNames.size() && !found; j++) {
                        found = returnBrowseNames[j] ==
                                UaQualifiedName(referenceDescriptions[i].BrowseName);
                    }
                    if (found) {
                        // skip variable
                        continue;
                    }
                    // convert UaNodeId to NodeId
                    NodeId* nid = converter->convertUa2io(nodeId); // ConversionException
                    ScopeGuard<NodeId> nidSG(nid);
                    Variant* v = NULL;
                    // if default event field "SourceNode"
                    if (0 == nodeId.namespaceIndex()
                            && OpcUaId_BaseEventType_SourceNode == nodeId.identifierNumeric()) {
                        // get a source node for the event
                        UaNodeId* sourceNode = getEventSourceNode(rootObjectType); // GeneratorException
                        if (sourceNode == NULL) {
                            std::ostringstream msg;
                            msg << "Cannot find a source node for event type "
                                    << rootObjectType.nodeId().toXmlString().toUtf8();
                            throw ExceptionDef(GeneratorException, msg.str());
                        }
                        // convert UaNodeId to Variant
                        UaVariant value;
                        value.setNodeId(*sourceNode);
                        v = converter->convertUa2io(value, UaNodeId(OpcUaType_NodeId)); // ConversionException                            
                    } else {
                        // generate UaVariant value for event field
                        variable = nodeBrowser->getVariable(nodeId);
                        OpcUa_Boolean isArray = variable->valueRank() == 1;
                        UaVariant* value = generate(variable->dataType(), isArray,
                                0 /*indent*/); // ConversionExceptio, GeneratorExceptionn                
                        ScopeGuard<UaVariant> valueSG(value);
                        // convert UaVariant to Variant
                        v = converter->convertUa2io(*value, variable->dataType()); // ConversionException
                        variable->releaseReference();
                    }
                    returnBrowseNames.push_back(UaQualifiedName(referenceDescriptions[i].BrowseName));
                    // add field data to list                    
                    returnEventFieldData.push_back(
                            new NodeData(*nidSG.detach(), v, true /*attachValues*/));
                }
            } catch (Exception& e) {
                if (variable != NULL) {
                    variable->releaseReference();
                }
                // process as many event fields as possible
                std::string st;
                e.getStackTrace(st);
                log->error("Exception while generating event field data: %s", st.c_str());
            }
        }
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
            UaObjectType* superObjectType = NULL;
            try {
                // if super type or sub type
                if (OpcUaId_HasSubtype
                        == UaNodeId(referenceDescriptions[i].ReferenceTypeId).identifierNumeric()) {
                    // if super type
                    if (!referenceDescriptions[i].IsForward) {
                        superObjectType = nodeBrowser->getObjectType(nodeId);
                        // get fields of super type
                        generateEventFieldData(rootObjectType, *superObjectType,
                                returnBrowseNames, returnEventFieldData); // ConversionException, GeneratorException
                        superObjectType->releaseReference();
                    }
                    // childs are ignored
                }
            } catch (Exception& e) {
                if (superObjectType != NULL) {
                    superObjectType->releaseReference();
                }
                // process as many event fields as possible
                std::string st;
                e.getStackTrace(st);
                log->error("Exception while generating object type data: %s", st.c_str());
            }
        }
    }

    UaNodeId * DataGeneratorPrivate::getEventSourceNode(UaObjectType & objectType)
    /* throws GeneratorExceptionn */ {
        // Part 3 4.5.3: Subtyping        
        // Part 4 B1.3: example for a content filter to select an event type including derived types
        ServiceContext sc;
        UaNodeId nodeToBrowse;
        UaNodeId referenceTypeId(OpcUaId_References);
        BrowseContext bc(NULL /*view*/,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
                0 /*maxResultsToReturn*/,
                OpcUa_BrowseDirection_Inverse,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
                OpcUa_True /*includeSubtypes*/,
                0 /* OpcUa_NodeClass_ObjectType*/ /*nodeClassMask*/,
                OpcUa_BrowseResultMask_All /* resultMask */);
        UaReferenceDescriptions referenceDescriptions;
        UaStatus result = objectType.browse(sc, bc, referenceDescriptions);
        if (!result.isGood()) {
            std::ostringstream msg;
            msg << "Cannot get event fields for " << objectType.nodeId().toXmlString().toUtf8()
                    << ": " << result.toString().toUtf8();
            throw ExceptionDef(GeneratorException, msg.str());
        }
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
            UaNodeId refId(referenceDescriptions[i].ReferenceTypeId);
            UaNode* node = NULL;
            try {
                UaNodeId* sourceNode = NULL;
                // if super type
                if (OpcUaId_HasSubtype == refId.identifierNumeric()) {
                    // get an event source node from super type
                    UaObjectType* superObjectType = nodeBrowser->getObjectType(nodeId);
                    node = superObjectType;
                    sourceNode = getEventSourceNode(*superObjectType); // GeneratorExceptionn
                } else if (OpcUaId_GeneratesEvent == refId.identifierNumeric()) {
                    // get all sub types of the source type
                    node = nodeBrowser->getNode(nodeId);
                    std::vector<UaNodeId*>* types = new std::vector<UaNodeId*>();
                    VectorScopeGuard<UaNodeId> typesSG(types);
                    getSubTypes(*node, *types); // GeneratorException
                    node->releaseReference();
                    // add source type to types
                    types->push_back(new UaNodeId(nodeId));
                    // get instance for source type
                    node = nodeBrowser->getNode(UaNodeId(OpcUaId_ObjectsFolder));
                    sourceNode = getInstance(*node, *types); // GeneratorException
                }
                if (node != NULL) {
                    node->releaseReference();
                }
                if (sourceNode != NULL) {
                    return sourceNode;
                }
            } catch (Exception& e) {
                if (node != NULL) {
                    node->releaseReference();
                }
                throw;
            }
        }
        return NULL;
    }

    UaNodeId * DataGeneratorPrivate::getInstance(UaNode& root, std::vector<UaNodeId*>& types)
    /* throws GeneratorExceptionn */ {
        // use internal client API for browsing (root.browse only returns objects of namespace 0)
        UaReferenceDescriptions referenceDescriptions;
        ContinuationPointWrapper cpw;
        UaStatus result = serverManager->browse(internalSession,
                root.nodeId() /*startingNode*/, false /*isInverse*/, UaNodeId(OpcUaId_References),
                0 /*nodeClassMask*/, cpw, referenceDescriptions);
        if (!result.isGood()) {
            std::ostringstream msg;
            msg << "Cannot get instances for types";
            for (int i = 0; i < types.size(); i++) {
                msg << " " << (*types[i]).toXmlString().toUtf8();
            }
            msg << ": " << result.toString().toUtf8();
            throw ExceptionDef(GeneratorException, msg.str());
        }
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
            UaNodeId refId(referenceDescriptions[i].ReferenceTypeId);
            UaNodeId* instance = NULL;
            switch (refId.identifierNumeric()) {
                case OpcUaId_HasTypeDefinition:
                    for (int i = 0; i < types.size(); i++) {
                        UaNodeId& type = *types[i];
                        if (type == nodeId) {
                            instance = new UaNodeId(root.nodeId());
                            break;
                        }
                    }
                    break;
                    // HierarchicalReferences:
                case OpcUaId_Organizes: // 35
                case OpcUaId_HasProperty: // 46
                case OpcUaId_HasComponent: // 47
                case OpcUaId_HasOrderedComponent: // 49 
                {
                    UaNode* node = nodeBrowser->getNode(nodeId);
                    try {
                        // TODO nodes of server namespace 1 are not found
                        if (node != NULL) {
                            instance = getInstance(*node, types); // GeneratorException
                            node->releaseReference();
                        }
                    } catch (Exception& e) {
                        if (node != NULL) {
                            node->releaseReference();
                        }
                    }
                    break;
                }
            }
            if (instance != NULL) {
                return instance;
            }
        }
        return NULL;
    }

    void DataGeneratorPrivate::getSubTypes(UaNode& type, std::vector<UaNodeId*>& returnSubTypes)
    /* throws GeneratorException */ {
        ServiceContext sc;
        UaNodeId nodeToBrowse;
        UaNodeId referenceTypeId(OpcUaId_HasSubtype);
        BrowseContext bc(NULL /*view*/,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) nodeToBrowse,
                0 /*maxResultsToReturn*/,
                OpcUa_BrowseDirection_Forward,
                (OpcUa_NodeId*) (const OpcUa_NodeId*) referenceTypeId,
                OpcUa_True /*includeSubtypes*/,
                0 /*nodeClassMask*/,
                OpcUa_BrowseResultMask_All /* resultMask */);
        UaReferenceDescriptions referenceDescriptions;
        UaStatus result = type.browse(sc, bc, referenceDescriptions);
        if (!result.isGood()) {
            std::ostringstream msg;
            msg << "Cannot get sub types for " << type.nodeId().toXmlString().toUtf8()
                    << ": " << result.toString().toUtf8();
            throw ExceptionDef(GeneratorException, msg.str());
        }
        for (OpcUa_UInt32 i = 0; i < referenceDescriptions.length(); i++) {
            UaNodeId* nodeId = new UaNodeId(referenceDescriptions[i].NodeId.NodeId);
            ScopeGuard<UaNodeId> nodeIdSG(nodeId);
            UaNode* node = nodeBrowser->getNode(*nodeId);
            try {
                returnSubTypes.push_back(nodeIdSG.detach());
                getSubTypes(*node, returnSubTypes); // GeneratorException                
                node->releaseReference();
            } catch (Exception& e) {
                node->releaseReference();
                throw;
            }
        }
    }

    UaVariant * DataGeneratorPrivate::generate(const UaNodeId& dataTypeId, OpcUa_Boolean isArray,
            OpcUa_Int16 indent) /* throws ConversionException, GeneratorException */ {
        if (isArray) {
            return generateArray(dataTypeId, indent); // ConversionException, GeneratorException
        }
        UaNodeId buildInDataTypeId = getBuildInType(dataTypeId); // GeneratorException
        switch (buildInDataTypeId.identifierNumeric()) {
            case OpcUaId_Structure: // 22
            case OpcUaId_Union: // 12756
                return generateStructure(dataTypeId, indent); // ConversionException, GeneratorException
            default:
                return generateBuildInType(buildInDataTypeId, indent); // GeneratorException
        }
    }

    UaVariant * DataGeneratorPrivate::generateBuildInType(const UaNodeId& dataTypeId,
            OpcUa_Int16 indent) /* throws GeneratorException */ {
        UaVariant* ret = NULL;
        if (0 == dataTypeId.namespaceIndex()) {
            switch (dataTypeId.identifierNumeric()) {
                case OpcUaType_Boolean: // 1
                    ret = new UaVariant();
                    ret->setBool(true);
                    break;
                case OpcUaType_SByte: // 2
                    ret = new UaVariant();
                    ret->setSByte(valueCounter++);
                    break;
                case OpcUaType_Byte: // 3
                    ret = new UaVariant();
                    ret->setByte(valueCounter++);
                    break;
                case OpcUaType_Int16: // 4
                    ret = new UaVariant();
                    ret->setInt16(valueCounter++);
                    break;
                case OpcUaType_UInt16: // 5
                    ret = new UaVariant();
                    ret->setUInt16(valueCounter++);
                    break;
                case OpcUaType_Int32: // 6                
                    ret = new UaVariant();
                    ret->setInt32(valueCounter++);
                    break;
                case OpcUaType_UInt32: // 7
                    ret = new UaVariant();
                    ret->setUInt32(valueCounter++);
                    break;
                case OpcUaType_Int64: // 8
                    ret = new UaVariant();
                    ret->setInt64(valueCounter++);
                    break;
                case OpcUaType_UInt64: // 9
                    ret = new UaVariant();
                    ret->setUInt64(valueCounter++);
                    break;
                case OpcUaType_Float: // 10
                    ret = new UaVariant();
                    ret->setFloat(valueCounter + valueCounter++ / 1000.0);
                    break;
                case OpcUaType_Double: // 11
                case OpcUaId_Duration: // 290
                    ret = new UaVariant();
                    ret->setDouble(valueCounter + valueCounter++ / 1000.0);
                    break;
                case OpcUaType_String: // 12
                {
                    ret = new UaVariant();
                    UaString s("string");
                    s += UaString::number(valueCounter++);
                    ret->setString(s);
                    break;
                }
                case OpcUaType_DateTime: // 13
                case OpcUaId_UtcTime: // 294
                    ret = new UaVariant();
                    ret->setDateTime(UaDateTime::now());
                    break;
                case OpcUaType_ByteString: // 15
                {
                    ret = new UaVariant();
                    OpcUa_Byte b[2];
                    b[0] = valueCounter++;
                    b[1] = valueCounter++;
                    UaByteString bs(2, b);
                    ret->setByteString(bs, true /*detach*/);
                    break;
                }
                case OpcUaId_LocalizedText: // 21
                {
                    ret = new UaVariant();
                    UaString s("string");
                    s += UaString::number(valueCounter++);
                    UaLocalizedText lt(UaString("en"), s);
                    ret->setLocalizedText(lt);
                    break;
                }
                case OpcUaId_Enumeration: // 29
                    ret = new UaVariant();
                    ret->setInt32(0);
                    break;
                default:
                    std::ostringstream msg;
                    msg << "Cannot generate data for type " << dataTypeId.toXmlString().toUtf8();
                    throw ExceptionDef(GeneratorException, msg.str());
            }
        }
        return ret;
    }

    UaVariant * DataGeneratorPrivate::generateStructure(
            const UaNodeId& dataTypeId, OpcUa_Int16 indent) /* throws ConversionException, GeneratorExceptionn */ {
        UaStructureDefinition sd = nodeBrowser->getStructureDefinition(dataTypeId);
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("%sstruct name=%s,dataTypeId=%s,isUnion=%d", ind,
                    sd.name().toUtf8(), sd.dataTypeId().toXmlString().toUtf8(), sd.isUnion());
        }
        UaExtensionObject eo;
        if (sd.isUnion()) {
            UaGenericUnionValue uv(sd);
            UaVariant* fieldValue = NULL;
            GeneratorException* exception = NULL;
            for (int j = 0; j < sd.childrenCount() && fieldValue == NULL; j++) {
                UaStructureField field = sd.child(j);
                try {
                    fieldValue = generateStructureField(field,
                            indent + 1); // ConversionException, GeneratorException
                    uv.setValue(field.name(), *fieldValue, false /*detach*/);
                } catch (Exception& e) {
                    if (exception == NULL) {
                        exception = new ExceptionDef(GeneratorException,
                                std::string("Cannot generate data for union field ")
                                .append(field.name().toUtf8())
                                .append(" of union type ").append(dataTypeId.toXmlString().toUtf8()));
                        exception->setCause(&e);
                    }
                }
            }
            if (fieldValue != NULL) {
                delete fieldValue;
                delete exception;
            } else if (exception != NULL) {
                ScopeGuard<GeneratorException> exceptionSG(exception);
                throw *exception;
            }
            uv.toExtensionObject(eo);
        } else {
            UaGenericStructureValue sv(sd);
            for (int j = 0; j < sd.childrenCount(); j++) {
                UaStructureField field = sd.child(j);
                try {
                    UaVariant* fieldValue = generateStructureField(field,
                            indent + 1); // ConversionException, GeneratorException
                    sv.setField(field.name(), *fieldValue);
                    delete fieldValue;

                    //                    UaExtensionObject a;
                    //                    sv.toExtensionObject(a);
                    //                    UaGenericStructureValue b(a, sd);
                    //                    fprintf(stderr, "############ %s %d\n", field.name().toUtf8(), b.isFieldSet(field.name()));

                } catch (Exception& e) {
                    if (field.isOptional()) {
                        GeneratorException ex = ExceptionDef(GeneratorException,
                                std::string("Cannot generate data for optional structure field ")
                                .append(field.name().toUtf8())
                                .append(" of structure type ").append(dataTypeId.toXmlString().toUtf8()));
                        ex.setCause(&e);
                        std::string st;
                        ex.getStackTrace(st);
                        log->error("Exception while generating data: %s", st.c_str());
                    } else {
                        throw;
                    }
                }
            }
            sv.toExtensionObject(eo);
        }
        UaVariant* ret = new UaVariant();
        ret->setExtensionObject(eo, OpcUa_False /*detach*/);

        return ret;
    }

    UaVariant * DataGeneratorPrivate::generateStructureField(
            UaStructureField& field, OpcUa_Int16 indent) /* throws ConversionException, GeneratorExceptionn */ {
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("%sfield name=%s,dataTypeId=%s,variantType=%d,arrayType=%d", ind,
                    field.name().toUtf8(), field.typeId().toXmlString().toUtf8(), field.valueType(),
                    field.arrayType());
        }
        OpcUa_Boolean isArray = field.arrayType() == OpcUa_VariantArrayType_Array;
        UaVariant* fieldValue = generate(field.typeId(), isArray,
                indent); // ConversionException, GeneratorExceptionn
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            // convert UaVariant to Variant
            Variant* v = converter->convertUa2io(*fieldValue, field.typeId()); // ConversionException
            ScopeGuard<Variant> vSG(v);
            log->trace("%s-> type=%d,value=%s", ind, fieldValue->type(), v->toString().c_str());
        }
        return fieldValue;
    }

    UaVariant * DataGeneratorPrivate::generateArray(const UaNodeId& dataTypeId,
            OpcUa_Int16 indent) /* throws ConversionException, GeneratorExceptionn */ {
        OpcUa_Int16 arraySize = 1;
        std::vector<UaVariant*>* values = new std::vector<UaVariant*>();
        VectorScopeGuard<UaVariant> valuesSG(values);
        for (OpcUa_Int16 i = 0; i < arraySize; i++) {
            UaVariant* v = generate(dataTypeId, false /* isArray */,
                    indent + 1); // ConversionException, GeneratorExceptionn            
            values->push_back(v);
        }
        switch (values->front()->type()) {
            case OpcUaType_ExtensionObject:
                return convert2ExtensionObjectArray(*values);
            case OpcUaType_Int32:
                return convert2Int32Array(*values);
            case OpcUaType_UInt32:
                return convert2UInt32Array(*values);
            case OpcUaType_LocalizedText:
                return convert2LocalizedTextArray(*values);
            default:
                std::ostringstream msg;
                msg << "Unsupported array data type: " << dataTypeId.toXmlString().toUtf8();
                throw ExceptionDef(GeneratorException, msg.str());
        }
    }

    UaVariant * DataGeneratorPrivate::convert2ExtensionObjectArray(
            std::vector<UaVariant*>& values) {
        UaExtensionObjectArray array;
        array.create(values.size());
        for (OpcUa_Int16 i = 0; i < values.size(); i++) {
            UaExtensionObject value;
            values.at(i)->toExtensionObject(value);
            value.copyTo(&array[i]);
        }
        UaVariant* ret = new UaVariant();
        ret->setExtensionObjectArray(array, OpcUa_True /*detach*/);

        return ret;
    }

    UaVariant * DataGeneratorPrivate::convert2Int32Array(
            std::vector<UaVariant*>& values) {
        UaInt32Array array;
        array.create(values.size());
        for (OpcUa_Int16 i = 0; i < values.size(); i++) {
            OpcUa_Int32 value;
            values.at(i)->toInt32(value);
            array[i] = value;
        }
        UaVariant* ret = new UaVariant();
        ret->setInt32Array(array, OpcUa_True /*detach*/);
        return ret;
    }
    
    UaVariant * DataGeneratorPrivate::convert2UInt32Array(
            std::vector<UaVariant*>& values) {
        UaUInt32Array array;
        array.create(values.size());
        for (OpcUa_Int16 i = 0; i < values.size(); i++) {
            OpcUa_UInt32 value;
            values.at(i)->toUInt32(value);
            array[i] = value;
        }
        UaVariant* ret = new UaVariant();
        ret->setUInt32Array(array, OpcUa_True /*detach*/);
        return ret;
    }
    
    UaVariant * DataGeneratorPrivate::convert2LocalizedTextArray(
            std::vector<UaVariant*>& values) {
        UaLocalizedTextArray array;
        array.create(values.size());
        for (OpcUa_Int16 i = 0; i < values.size(); i++) {
            UaLocalizedText value;
            values.at(i)->toLocalizedText(value);
            value.copyTo(&array[i]);
        }
        UaVariant* ret = new UaVariant();
        ret->setLocalizedTextArray(array, OpcUa_True /*detach*/);
        return ret;
    }
} // namespace SASModelProviderNamespace
