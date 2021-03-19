#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include "generator/GeneratorIODataProvider.h"
#include <common/Exception.h>
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/MethodData.h>
#include <ioDataProvider/NodeData.h>
#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/NodeProperties.h>
#include <ioDataProvider/Structure.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridgeException.h>
#include <sasModelProvider/base/ConversionException.h>
#include <sasModelProvider/base/ConverterUa2IO.h>
#include <sasModelProvider/base/IODataProviderSubscriberCallback.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <methodhandleuanode.h> // MethodHandleUaNode
#include <statuscode.h> // UaStatus
#include <uaargument.h> // UaArgument
#include <uaarraytemplates.h> // UaStatusCodeArray
#include <uabasenodes.h> // UaVariable
#include <uadatetime.h> // UaDateTime
#include <uadiagnosticinfos.h> // UaDiagnosticInfos
#include <uanodeid.h> // UaNodeId
#include <uastring.h> // UaString
#include <uavariant.h> // UaVariant
#include <iterator>
#include <map>
#include <sstream> // std::ostringstream
#include <string>
#include <vector>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

//#define USE_DATA_GENERATOR

using namespace CommonNamespace;
using namespace IODataProviderNamespace;

namespace SASModelProviderNamespace {

    class HaNodeManagerIODataProviderBridgePrivate {
        friend class HaNodeManagerIODataProviderBridge;
    private:

        class ConverterCallback : public ConverterUa2IO::ConverterCallback {
        public:

            ConverterCallback(NodeBrowser& nodeBrowser) {
                this->nodeBrowser = &nodeBrowser;
            }

            virtual UaStructureDefinition getStructureDefinition(const UaNodeId& dataTypeId) {
                return nodeBrowser->getStructureDefinition(dataTypeId);
            }

            virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& nodeId) {
                return nodeBrowser->getSuperTypes(nodeId);
            }

        private:
            NodeBrowser* nodeBrowser;
        };

        Logger* log;

        HaNodeManager* haNodeManager;
        NodeBrowser* nodeBrowser;
        IODataProviderNamespace::IODataProvider* ioDataProvider;
        IODataProviderSubscriberCallback* ioDataProviderSubscriberCallback;
        const IODataProviderNamespace::NodeProperties* dfltNodeProps;
        std::vector<const IODataProviderNamespace::NodeData*>* nodePropsMap;
        ConverterUa2IO* converter;
        static GeneratorIODataProvider* dataGenerator;

        // Gets the value handling for a node.
        NodeProperties::ValueHandling getValueHandling(
                const IODataProviderNamespace::NodeId& nodeId);
    };

    GeneratorIODataProvider* HaNodeManagerIODataProviderBridgePrivate::dataGenerator = NULL;

    HaNodeManagerIODataProviderBridge::HaNodeManagerIODataProviderBridge(
            HaNodeManager& haNodeManager, IODataProvider& ioDataProvider) {
        d = new HaNodeManagerIODataProviderBridgePrivate();
        d->log = LoggerFactory::getLogger("HaNodeManagerIODataProviderBridge");
        d->haNodeManager = &haNodeManager;
        d->nodeBrowser = NULL;
        d->ioDataProvider = &ioDataProvider;
        d->ioDataProviderSubscriberCallback = NULL;
        d->dfltNodeProps = NULL;
        d->nodePropsMap = NULL;
        d->converter = NULL;
    }

    HaNodeManagerIODataProviderBridge::~HaNodeManagerIODataProviderBridge() {
        delete d;
    }

    UaStatus HaNodeManagerIODataProviderBridge::afterStartUp() {
        std::string ns(d->haNodeManager->getNameSpaceUri().toUtf8());
        OpcUa_UInt16 nsIndex = d->haNodeManager->getNodeManagerBase().getNameSpaceIndex();
        try {
            d->nodeBrowser = new NodeBrowser(*d->haNodeManager);
            d->ioDataProviderSubscriberCallback = new IODataProviderSubscriberCallback(
                    *d->haNodeManager);
            d->dfltNodeProps = d->ioDataProvider->getDefaultNodeProperties(ns,
                    nsIndex); // IODataProviderException
            d->nodePropsMap = d->ioDataProvider->getNodeProperties(ns,
                    nsIndex); // IODataProviderException
            d->converter = new ConverterUa2IO(
                    *new HaNodeManagerIODataProviderBridgePrivate::ConverterCallback(
                    *d->nodeBrowser), true /* attachValues*/);
#ifdef USE_DATA_GENERATOR
            if (d->dataGenerator == NULL) {
                d->dataGenerator = new GeneratorIODataProvider(*d->nodeBrowser, *d->converter);
            }
#endif
            d->log->info("Started bridge from node manager for namespace index %d to IO data provider",
                    nsIndex);
            return UaStatus(OpcUa_Good);
        } catch (Exception& e) {
            std::ostringstream msg;
            msg << "Start up of bridge from node manager for namespace " << ns
                    << " with index " << nsIndex << " to IO data provider failed";
            HaNodeManagerIODataProviderBridgeException ex =
                    ExceptionDef(HaNodeManagerIODataProviderBridgeException, msg.str());
            ex.setCause(&e);
            // there is no way to inform the OPC UA server about details => log the exception
            std::string st;
            ex.getStackTrace(st);
            d->log->error("Exception while starting brigde: %s", st.c_str());
            return UaStatus(OpcUa_Bad);
        }
    }

    UaStatus HaNodeManagerIODataProviderBridge::beforeShutDown() {
        delete d->dataGenerator;
        d->dataGenerator = NULL;
        delete d->converter;
        if (d->nodePropsMap != NULL) {
            for (std::vector<const NodeData*>::const_iterator i =
                    d->nodePropsMap->begin(); i != d->nodePropsMap->end(); i++) {
                delete *i;
            }
            delete d->nodePropsMap;
        }
        if (d->dfltNodeProps != NULL) {
            delete d->dfltNodeProps;
        }
        delete d->ioDataProviderSubscriberCallback;
        delete d->nodeBrowser;
        return UaStatus(OpcUa_Good);
    }

    UaStatus HaNodeManagerIODataProviderBridge::readValues(const UaVariableArray &variables,
            UaDataValueArray &returnValues) {
        UaStatus ret(OpcUa_Good);
        // initialize all return values with status "bad"
        OpcUa_UInt32 variableCount = variables.length();
        returnValues.create(variableCount);
        UaDateTime serverTimeStamp = UaDateTime::now();
        for (OpcUa_UInt32 i = 0; i < variableCount; i++) {
            returnValues[i].setServerTimestamp(serverTimeStamp);
            returnValues[i].setStatusCode(OpcUa_Bad);
        }
        std::vector<const NodeId*>* nodeIds = new std::vector<const NodeId*>();
        VectorScopeGuard<const NodeId> nodeIdsSG(nodeIds);
        std::map<const NodeId*, OpcUa_UInt32> arrayIndices;
        HaNodeManagerIODataProviderBridgeException* exception = NULL;
        // for each variable
        for (OpcUa_UInt32 i = 0; i < variableCount; i++) {
            UaVariable& variable = *variables[i];
            try {
                // convert UaNodeId to NodeId
                NodeId* nodeId = d->converter->convertUa2io(variable.nodeId()); // ConversionException                    
                ScopeGuard<NodeId> nodeIdSG(nodeId);
                // if value handling is enabled
                if (d->getValueHandling(*nodeId) != NodeProperties::NONE) {
                    // save nodeId, array index
                    nodeIds->push_back(nodeIdSG.detach());
                    arrayIndices[nodeId] = i;
                } else if (d->log->isInfoEnabled()) {
                    d->log->info("READ %-20s Ignoring variable due to disabled value handling",
                            variable.nodeId().toXmlString().toUtf8());
                }
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            std::string("Cannot prepare the reading of values via IO data provider for ")
                            .append(variable.nodeId().toXmlString().toUtf8()));
                    exception->setCause(&e);
                }
            }
        }
        if (nodeIds->size() > 0) {
            try {
                // get values from IO data provider
                std::vector<NodeData*>* results = d->dataGenerator == NULL ?
                        d->ioDataProvider->read(*nodeIds) // IODataProviderException                     
                        : d->dataGenerator->read(*nodeIds);
                VectorScopeGuard<NodeData> resultsSG(results);
                OpcUa_UInt32 resultCount = results == NULL ? 0 : results->size();
                if (resultCount != nodeIds->size() && exception == NULL) {
                    std::ostringstream msg;
                    msg << "Invalid count of node data returned from IO data provider: "
                            << resultCount << "/" << nodeIds->size();
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            msg.str());
                }
                serverTimeStamp = UaDateTime::now();
                // for each returned nodeId
                for (int i = 0; i < resultCount; i++) {
                    NodeData& result = *(*results)[i];
                    const NodeId& nodeId = result.getNodeId();
                    // get array index for nodeId
                    OpcUa_UInt32 arrayIndex = -1;
                    for (std::map<const NodeId*, OpcUa_UInt32>::const_iterator it =
                            arrayIndices.begin(); it != arrayIndices.end();
                            it++) {
                        if ((*it).first->equals(nodeId)) {
                            arrayIndex = (*it).second;
                            break;
                        }
                    }
                    if (arrayIndex < 0) {
                        if (exception == NULL) {
                            exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                    std::string("Received unrequested nodeId ")
                                    .append(nodeId.toString().c_str())
                                    .append(" while reading data from IO data provider"));
                        }
                        // continue with next nodeId
                        continue;
                    }
                    UaVariable& variable = *variables[arrayIndex];
                    const OpcUa_Variant& cacheValue = *variable.value(
                            NULL /* session */).value();
                    IODataProviderException* resultException = result.getException();
                    if (resultException == NULL) {
                        try {
                            // convert Variant to UaVariant
                            UaVariant* value = result.getData() == NULL ?
                                    new UaVariant() :
                                    d->converter->convertIo2ua(*result.getData(),
                                    variables[arrayIndex]->dataType()); // ConversionException
                            ScopeGuard<UaVariant> valueSG(value);
                            // set the value to the array
                            returnValues[arrayIndex].setValue(*value,
                                    OpcUa_False /* detachValue */,
                                    OpcUa_False /* updateTimeStamps */);
                            returnValues[arrayIndex].setStatusCode(OpcUa_Good);
                            if (d->log->isInfoEnabled()) {
                                d->log->info("READ %-20s nodeId=%s,oldValue=%s,newValue=%s",
                                        variable.browseName().toString().toUtf8(),
                                        variable.nodeId().toXmlString().toUtf8(),
                                        UaVariant(cacheValue).toFullString().toUtf8(),
                                        value->toFullString().toUtf8());
                            }
                        } catch (Exception& e) {
                            resultException = new ExceptionDef(IODataProviderException,
                                    std::string("Cannot set value for variable ")
                                    .append(nodeId.toString().c_str())
                                    .append(" after reading data from IO data provider"));
                            resultException->setCause(&e);
                        }
                    }
                    if (resultException != NULL) {
                        // there is no way to inform the OPC UA server about details => log the exception                        
                        std::string st;
                        resultException->getStackTrace(st);
                        d->log->error("READ %-20s nodeId=%s,oldValue=%s,exception=%s",
                                variable.browseName().toString().toUtf8(),
                                variable.nodeId().toXmlString().toUtf8(),
                                UaVariant(cacheValue).toFullString().toUtf8(),
                                st.c_str());
                        if (resultException != result.getException()) {
                            delete resultException;
                        }
                        returnValues[arrayIndex].setStatusCode(OpcUa_Bad);
                    }
                    returnValues[arrayIndex].setSourceTimestamp(serverTimeStamp);
                    returnValues[arrayIndex].setServerTimestamp(serverTimeStamp);
                } // for each returned nodeId                            
            } catch (Exception& e) {
                exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                        std::string("Cannot read values"));
                exception->setCause(&e);
            }
        } // if nodeIds->size() > 0
        if (exception != NULL) {
            HaNodeManagerIODataProviderBridgeException ex =
                    ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                    std::string("The reading of node values from IO data provider failed"));
            ex.setCause(exception);
            delete exception;
            // there is no way to inform the OPC UA server about details => log the exception
            std::string st;
            ex.getStackTrace(st);
            d->log->error("Exception while reading values: %s", st.c_str());
            ret = UaStatus(OpcUa_Bad);
        }
        return ret;
    }

    UaStatus HaNodeManagerIODataProviderBridge::writeValues(const UaVariableArray &variables,
            const PDataValueArray &values, UaStatusCodeArray &returnStatusCodes) {
        UaStatus ret(OpcUa_Good);
        // initialize return values
        OpcUa_UInt32 variableCount = variables.length();
        returnStatusCodes.create(variableCount);
        for (OpcUa_UInt32 i = 0; i < variableCount; i++) {
            returnStatusCodes[i] = OpcUa_Bad;
        }
        std::vector<const NodeData*>* nodeData = new std::vector<const NodeData*>();
        VectorScopeGuard<const NodeData> nodeDataSG(nodeData);
        HaNodeManagerIODataProviderBridgeException* exception = NULL;
        // get nodeData
        for (OpcUa_UInt32 i = 0; i < variableCount; i++) {
            UaVariable& variable = *variables[i];
            UaNodeId nodeId = variable.nodeId();
            try {
                UaVariant value(values[i]->Value);
                // convert UaNodeId to NodeId
                NodeId* nid = d->converter->convertUa2io(variable.nodeId()); // ConversionException  
                ScopeGuard<NodeId> nidSG(nid);
                // if value handling is enabled
                if (d->getValueHandling(*nid) != NodeProperties::NONE) {
                    const OpcUa_Variant& cacheValue = *variable.value(
                            NULL /* session */).value();
                    if (d->log->isInfoEnabled()) {
                        d->log->info("WRITE %-20s nodeId=%s,oldValue=%s,newValue=%s",
                                variable.browseName().toString().toUtf8(),
                                variable.nodeId().toXmlString().toUtf8(),
                                UaVariant(cacheValue).toFullString().toUtf8(),
                                value.toFullString().toUtf8());
                    }
                    // convert UaVariant to Variant
                    Variant* v = d->converter->convertUa2io(value, variable.dataType()); // ConversionException
                    // save nodeData                    
                    nodeData->push_back(new NodeData(*nidSG.detach(), v, true /* attachValues */));
                } else if (d->log->isInfoEnabled()) {
                    d->log->info("WRITE %-20s Ignoring variable due to disabled value handling",
                            variable.nodeId().toXmlString().toUtf8());
                }
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            std::string("Cannot prepare writing data to IO data provider for ")
                            .append(nodeId.toXmlString().toUtf8()));
                    exception->setCause(&e);
                }
            }
        }
        if (nodeData->size() > 0) {
            try {
                // write values to IO data provider
                d->ioDataProvider->write(*nodeData,
                        false /* sendValueChangedEvents */); // IODataProviderException                            
                // write values to the server cache
                for (OpcUa_UInt32 i = 0; i < variableCount; i++) {
                    UaVariable& variable = *variables[i];
                    try {
                        UaVariant srcValue(values[i]->Value);
                        d->haNodeManager->setVariable(variable, srcValue); // HaNodeManagerException
                        returnStatusCodes[i] = OpcUa_Good;
                    } catch (Exception& e) {
                        if (exception == NULL) {
                            exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                    std::string("Cannot write data to server cache for ")
                                    .append(variable.nodeId().toXmlString().toUtf8()));
                            exception->setCause(&e);
                        }
                    }
                }
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            std::string("Cannot write data to IO data provider"));
                    exception->setCause(&e);
                }
            }
        }
        if (exception != NULL) {
            HaNodeManagerIODataProviderBridgeException ex =
                    ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                    std::string("The writing of node values to IO data provider and server cache failed"));
            ex.setCause(exception);
            delete exception;
            // there is no way to inform the OPC UA server about details => log the exception
            std::string st;
            ex.getStackTrace(st);
            d->log->error("Exception while writing values: %s", st.c_str());
            ret = UaStatus(OpcUa_Bad);
        }
        return ret;
    }

    void HaNodeManagerIODataProviderBridge::afterSetAttributeValue(
            Session* pSession, UaNode* pNode, OpcUa_Int32 attributeId,
            const UaDataValue& dataValue) {
        UaNodeId variableNodeId = pNode->nodeId();
        UaVariable& variable = *d->nodeBrowser->getVariable(variableNodeId);
        // convert UaNodeId to NodeId
        NodeId* nodeId = d->converter->convertUa2io(variableNodeId); // ConversionException                    
        ScopeGuard<NodeId> nodeIdSG(nodeId);
        // if value handling is enabled
        if (d->getValueHandling(*nodeId) != NodeProperties::NONE) {
            if (d->log->isInfoEnabled()) {
                d->log->info("ASET %-20s nodeId=%s,value=%s",
                        variable.browseName().toString().toUtf8(),
                        variable.nodeId().toXmlString().toUtf8(),
                        UaVariant(*dataValue.value()).toFullString().toUtf8());
            }
            try {
                // convert UaVariant to Variant
                Variant* value = d->converter->convertUa2io(UaVariant(*dataValue.value()),
                        variable.dataType()); // ConversionException
                NodeData nd(*nodeIdSG.detach(), value, true /* attachValues */);
                std::vector<const NodeData*> nodeData;
                nodeData.push_back(&nd);
                d->ioDataProvider->write(nodeData,
                        false /* sendValueChangedEvents */); // IODataProviderException
            } catch (Exception& e) {
                std::ostringstream msg;
                msg << "The writing of node values to IO data provider after server cache modification failed";
                HaNodeManagerIODataProviderBridgeException ex =
                        ExceptionDef(HaNodeManagerIODataProviderBridgeException, msg.str());
                ex.setCause(&e);
                // there is no way to inform the OPC UA server => log the exception
                std::string st;
                ex.getStackTrace(st);
                d->log->error("Exception while writing values: %s", st.c_str());
            }
        } else if (d->log->isInfoEnabled()) {

            d->log->info("ASET %-20s Ignoring variable due to disabled value handling",
                    variable.nodeId().toXmlString().toUtf8());
        }
        variable.releaseReference();
    }

    void HaNodeManagerIODataProviderBridge::variableCacheMonitoringChanged(
            UaVariableCache* pVariable,
            IOManager::TransactionType transactionType) {
        if (d->log->isInfoEnabled()) {

            d->log->info("MON %-20s nodeId=%s,transactionType=%s",
                    pVariable->browseName().toString().toUtf8(),
                    pVariable->nodeId().toXmlString().toUtf8(),
                    transactionType == IOManager::TransactionMonitorBegin ?
                    "monitorBegin" : "monitorStop");
        }
    }

    UaStatus HaNodeManagerIODataProviderBridge::beginCall(
            MethodManagerCallback* callback, const ServiceContext& serviceContext,
            OpcUa_UInt32 callbackHandle, MethodHandle* methodHandle,
            const UaVariantArray & inputArgumentsValues) {
        UaStatus ret(OpcUa_Good);

        MethodHandleUaNode& methodHandleUaNode =
                *static_cast<MethodHandleUaNode*> (methodHandle);
        const UaNodeId& objectNodeId = methodHandleUaNode.pUaObject()->nodeId();
        const UaNodeId& methodNodeId = methodHandleUaNode.pUaMethod()->nodeId();
        if (d->log->isInfoEnabled()) {
            d->log->info("CALL %-20s on object %s", methodNodeId.toXmlString().toUtf8(),
                    objectNodeId.toXmlString().toUtf8());
        }
        UaReferenceLists* methodReferences = methodHandleUaNode.pUaMethod()->getUaReferenceLists();

        // input arguments                        
        std::vector<UaNodeId> inputArgsDataTypes;
        if (inputArgumentsValues.length() > 0) {
            UaVariable* argsVariable = static_cast<UaVariable*> (
                    methodReferences->getTargetNodeByBrowseName(
                    UaQualifiedName("InputArguments", 0 /*nsIndex*/)));
            UaVariant args(*argsVariable->value(NULL /*session*/).value());
            UaExtensionObjectArray argsEOA;
            args.toExtensionObjectArray(argsEOA);
            for (int i = 0; i < argsEOA.length(); i++) {
                UaArgument arg(argsEOA[i]);
                inputArgsDataTypes.push_back(arg.getDataType());
            }
        }
        std::vector<const Variant*>* inputArgsValues = new std::vector<const Variant*>();
        VectorScopeGuard<const Variant> inputArgsValuesSG(inputArgsValues);
        UaStatusCodeArray returnInputArgsStatusCodes;
        returnInputArgsStatusCodes.create(inputArgumentsValues.length());
        for (OpcUa_UInt32 i = 0; i < inputArgumentsValues.length(); i++) {
            returnInputArgsStatusCodes[i] = OpcUa_BadInvalidArgument;
        }
        UaDiagnosticInfos returnInputArgsDiags;

        // output arguments
        std::vector<UaArgument*>* outputArgsArguments = new std::vector<UaArgument*>();
        VectorScopeGuard<UaArgument> outputArgsDataTypesSG(outputArgsArguments);
        UaVariable* argsVariable = static_cast<UaVariable*> (
                methodReferences->getTargetNodeByBrowseName(
                UaQualifiedName("OutputArguments", 0 /*nsIndex*/)));
        if (argsVariable != NULL) {
            UaVariant args = UaVariant(*argsVariable->value(NULL /* session*/).value());
            UaExtensionObjectArray argsEOA;
            args.toExtensionObjectArray(argsEOA);
            for (int i = 0; i < argsEOA.length(); i++) {
                outputArgsArguments->push_back(new UaArgument(argsEOA[i]));
            }
        }
        UaVariantArray returnOutputArgsValues;
        returnOutputArgsValues.create(outputArgsArguments->size());

        try {
            for (OpcUa_UInt32 i = 0; i < inputArgumentsValues.length(); i++) {
                // convert value from UaVariant to Variant
                Variant* value = d->converter->convertUa2io(UaVariant(inputArgumentsValues[i]),
                        inputArgsDataTypes.at(i)); // ConversionException
                inputArgsValues->push_back(value);
                returnInputArgsStatusCodes[i] = OpcUa_Good;
            }
            // convert UaNodeIds to NodeIds (object + method)
            NodeId* destObjectNodeId = d->converter->convertUa2io(objectNodeId); // ConversionException
            ScopeGuard<NodeId> objectNodeIdSG(destObjectNodeId);
            NodeId* destMethodNodeId = d->converter->convertUa2io(methodNodeId); // ConversionException            
            // forward method call to IO data provider                
            MethodData md(*objectNodeIdSG.detach(), *destMethodNodeId,
                    *inputArgsValuesSG.detach(), true /* attachValues */);
            std::vector<const MethodData*> methodData;
            methodData.push_back(&md);
            std::vector<MethodData*>* results = d->dataGenerator == NULL ?
                    d->ioDataProvider->call(methodData) // IODataProviderException
                    : d->dataGenerator->call(methodData);
            VectorScopeGuard<MethodData> resultsSG(results);
            OpcUa_UInt32 resultCount = results == NULL ? 0 : results->size();
            if (resultCount != 1) {
                std::ostringstream msg;
                msg << "Invalid count of method data returned from IO data provider: "
                        << resultCount << "/1";
                throw ExceptionDef(HaNodeManagerIODataProviderBridgeException, msg.str());
            }
            MethodData* result = (*results)[0];
            if (result != NULL && result->getException() != NULL) {
                throw *result->getException();
            }
            if (result == NULL ||
                    result->getMethodArguments().size() != returnOutputArgsValues.length()) {
                std::ostringstream msg;
                msg << "Invalid count of method output arguments returned from IO data provider: "
                        << result->getMethodArguments().size() << "/"
                        << returnOutputArgsValues.length();
                throw ExceptionDef(HaNodeManagerIODataProviderBridgeException, msg.str());
            }
            // convert list of Variant to UaVariantArray
            for (size_t j = 0; j < result->getMethodArguments().size(); j++) {
                const Variant* value = result->getMethodArguments()[j];
                if (value == NULL) {
                    UaVariant().copyTo(&returnOutputArgsValues[j]);
                } else {
                    // convert Variant to UaVariant
                    UaVariant* outputArgValue = d->converter->convertIo2ua(*value,
                            outputArgsArguments->at(j)->getDataType()); // ConversionException
                    outputArgValue->copyTo(&returnOutputArgsValues[j]);
                    delete outputArgValue;
                }
            }
        } catch (Exception& e) {
            HaNodeManagerIODataProviderBridgeException ex =
                    ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                    std::string("Calling method ").append(methodNodeId.toXmlString().toUtf8())
                    .append(" on object ").append(objectNodeId.toXmlString().toUtf8())
                    .append(" failed"));
            ex.setCause(&e);
            // there is no way to inform the OPC UA server about details => log the exception
            std::string st;
            ex.getStackTrace(st);
            d->log->error("Exception while calling method: %s", st.c_str());
            const unsigned long* errorCode = e.getErrorCode();
            ret = UaStatus(errorCode == NULL || UaStatusCode(*errorCode).isGood() ?
                    OpcUa_Bad : *errorCode);
        }
        callback->finishCall(callbackHandle, returnInputArgsStatusCodes,
                returnInputArgsDiags, returnOutputArgsValues, ret);
        return UaStatus(OpcUa_Good);
    }

    void HaNodeManagerIODataProviderBridge::updateValueHandling(
            const std::vector<UaObjectType*>& objectTypes)
    /* throws HaNodeManagerIODataProviderBridgeException */ {
        std::vector<const NodeId*>* asyncNodeIds = new std::vector<const NodeId*>();
        VectorScopeGuard<const NodeId> asyncNodeIdsSG(asyncNodeIds);
        HaNodeManagerIODataProviderBridgeException* exception = NULL;
        // for each object type
        for (int i = 0; i < objectTypes.size(); i++) {
            UaObjectType& objectType = *objectTypes[i];
            try {
                // convert UaNodeId to NodeId
                NodeId* nodeId = d->converter->convertUa2io(objectType.nodeId()); // ConversionException
                ScopeGuard<NodeId> nodeIdSG(nodeId);
                // get value handling
                NodeProperties::ValueHandling valueHandling = d->getValueHandling(*nodeId); // HaNodeManagerIODataProviderBridgeException
                switch (valueHandling) {
                    case NodeProperties::NONE:
                        break;
                    case NodeProperties::ASYNC:
                        // add subscription for object type
                        asyncNodeIds->push_back(nodeIdSG.detach());
                        break;
                    case NodeProperties::SYNC:
                    default:
                        if (exception == NULL) {
                            exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                    std::string("Object type must be handled asynchronously for ")
                                    .append(nodeId->toString()));
                        }
                }
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            std::string("Cannot get value handling for object type ")
                            .append(objectType.nodeId().toXmlString().toUtf8()));
                    exception->setCause(&e);
                }
            }
        }
        if (asyncNodeIds->size() > 0) {
            try {
                // subscribe
                // discard returned data because the server cache is not updated
                // (like OPC_UA_EVENT_DATA handling in IODataProviderSubscriberCallback)
                std::vector<IODataProviderNamespace::NodeData*>* results =
                        d->dataGenerator == NULL ?
                        d->ioDataProvider->subscribe(*asyncNodeIds,
                        *d->ioDataProviderSubscriberCallback) // IODataProviderException
                        : d->dataGenerator->subscribe(*asyncNodeIds,
                        *d->ioDataProviderSubscriberCallback);
                VectorScopeGuard<IODataProviderNamespace::NodeData> resultsSG(results);
                OpcUa_UInt32 resultCount = results == NULL ? 0 : results->size();
                if (resultCount != asyncNodeIds->size() && exception == NULL) {
                    std::ostringstream msg;
                    msg << "Invalid count of object type results from IO data provider ("
                            << resultCount << "/" << asyncNodeIds->size() << ")";
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            msg.str());
                }
                // for each returned node data
                for (int i = 0; i < resultCount && exception == NULL; i++) {
                    NodeData* result = (*results)[i];
                    IODataProviderException* resultException = result->getException();
                    if (resultException != NULL) {
                        exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                std::string("Cannot subscribe for object type: ")
                                .append(result->getNodeId().toString()));
                        exception->setCause(resultException);
                    }
                }
            } catch (Exception& e) {
                exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                        std::string("Cannot subscribe for object types"));
                exception->setCause(&e);
            }
        }
        if (exception != NULL) {
            HaNodeManagerIODataProviderBridgeException ex =
                    ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                    std::string("Cannot subscribe for object types with asynchronous value handling"));
            ex.setCause(exception);
            delete exception;
            throw ex;
        }
    }

    void HaNodeManagerIODataProviderBridge::updateValueHandling(
            const std::vector<UaVariable*>& variables)
    /* throws HaNodeManagerIODataProviderBridgeException */ {
        std::vector<const NodeId*>* asyncNodeIds = new std::vector<const NodeId*>();
        VectorScopeGuard<const NodeId> asyncNodeIdsSG(asyncNodeIds);
        HaNodeManagerIODataProviderBridgeException* exception = NULL;
        std::map<const NodeId*, UaVariable*> asyncVariables;
        // for each variable
        for (int i = 0; i < variables.size(); i++) {
            UaVariable& variable = *variables[i];
            try {
                // convert UaNodeId to NodeId
                NodeId* nodeId = d->converter->convertUa2io(variable.nodeId()); // ConversionException
                ScopeGuard<NodeId> nodeIdSG(nodeId);
                // get value handling
                std::string vh;
                switch (d->getValueHandling(*nodeId)) { // HaNodeManagerIODataProviderBridgeException
                    case NodeProperties::NONE:
                        vh = std::string("none");
                        // the variable is not handled by the IO data provider
                        // => the server cache must be the data source for OPC UA clients
                        variable.setValueHandling(UaVariable_Value_CacheIsSource);
                        break;
                    case NodeProperties::ASYNC:
                        vh = std::string("async");
                        // add subscription for variable                        
                        asyncNodeIds->push_back(nodeIdSG.detach());
                        asyncVariables[nodeId] = &variable;
                        // the server cache is the data source for OPC UA clients
                        variable.setValueHandling(UaVariable_Value_CacheIsSource);
                        break;
                    case NodeProperties::SYNC:
                        vh = std::string("sync");
                        // the IO data provider is the data source for OPC UA clients
                        // the server only caches the values
                        // method "writeValues" updates the cache
                        variable.setValueHandling(UaVariable_Value_CacheIsUpdatedOnRequest);
                    default:
                        break;
                }
                if (d->log->isInfoEnabled()) {
                    d->log->info("VALH %-20s nodeId=%s,valueHandling=%s",
                            variable.browseName().toString().toUtf8(),
                            variable.nodeId().toXmlString().toUtf8(), vh.c_str());
                }
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            std::string("Cannot get value handling for variable ")
                            .append(variable.nodeId().toXmlString().toUtf8()));
                    exception->setCause(&e);
                }
            }
        }
        if (asyncNodeIds->size() > 0) {
            try {
                // subscribe for async nodeIds and get initial values
                std::vector<NodeData*>* results = d->dataGenerator == NULL ?
                        d->ioDataProvider->subscribe(*asyncNodeIds,
                        *d->ioDataProviderSubscriberCallback) // IODataProviderException                                
                        : d->dataGenerator->subscribe(*asyncNodeIds,
                        *d->ioDataProviderSubscriberCallback);
                VectorScopeGuard<NodeData> resultsSG(results);
                OpcUa_UInt32 resultCount = results == NULL ? 0 : results->size();
                if (resultCount != asyncNodeIds->size() && exception == NULL) {
                    std::ostringstream msg;
                    msg << "Invalid count of initial variable values returned from IO data provider: "
                            << resultCount << "/" << asyncNodeIds->size();
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            msg.str());
                }
                // for each returned node data
                for (int i = 0; i < resultCount; i++) {
                    NodeData* result = (*results)[i];
                    const NodeId& nodeId = result->getNodeId();
                    // get the variable
                    UaVariable* variable = NULL;
                    for (std::map<const NodeId*, UaVariable*>::iterator it =
                            asyncVariables.begin(); it != asyncVariables.end();
                            it++) {
                        if ((*it).first->equals(nodeId)) {
                            variable = (*it).second;
                            break;
                        }
                    }
                    if (variable == NULL) {
                        if (exception == NULL) {
                            exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                    std::string("Received unrequested nodeId ")
                                    .append(nodeId.toString().c_str())
                                    .append(" while reading data from IO data provider"));
                        }
                        // continue with next variable
                        continue;
                    }
                    if (result->getException() == NULL) {
                        try {
                            // convert Variant to UaVariant
                            UaVariant* value = result->getData() == NULL ?
                                    new UaVariant() :
                                    d->converter->convertIo2ua(*result->getData(),
                                    variable->dataType()); // ConversionException                            
                            ScopeGuard<UaVariant> valueSG(value);                            
                            // set the value to the server cache
                            d->haNodeManager->setVariable(*variable, *value); // HaNodeManagerException
                        } catch (Exception& e) {
                            if (exception == NULL) {
                                exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                        std::string("Cannot set initial value for variable ")
                                        .append(nodeId.toString().c_str()));
                                exception->setCause(&e);
                            }
                        }
                    } else if (exception == NULL) {
                        exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                                std::string("Reading of initial value for variable ")
                                .append(nodeId.toString().c_str()).append(" failed"));
                        exception->setCause(result->getException());
                    }
                } // for each returned node data            
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                            std::string("Cannot subscribe for variables"));
                    exception->setCause(&e);
                }
            }
        } // asyncNodeIds->size() > 0
        if (exception != NULL) {
            HaNodeManagerIODataProviderBridgeException ex =
                    ExceptionDef(HaNodeManagerIODataProviderBridgeException,
                    std::string("Cannot subscribe for variables with asynchronous value handling"));
            ex.setCause(exception);
            delete exception;
            throw ex;
        }
    }

    UaNodeId * HaNodeManagerIODataProviderBridge::convert(
            const NodeId & nodeId) const /* throws ConversionException */ {
        return d->converter->convertIo2ua(nodeId);
    }

    NodeId * HaNodeManagerIODataProviderBridge::convert(
            const UaNodeId & nodeId) const /* throws ConversionException */ {
        return d->converter->convertUa2io(nodeId);
    }

    UaVariant * HaNodeManagerIODataProviderBridge::convert(const Variant& value,
            const UaNodeId & dataTypeId) /* throws ConversionException */ {
        return d->converter->convertIo2ua(value, dataTypeId);
    }

    Variant * HaNodeManagerIODataProviderBridge::convert(
            const UaVariant& value, const UaNodeId & dataTypeId) /* throws ConversionException */ {
        return d->converter->convertUa2io(value, dataTypeId);
    }

    NodeProperties::ValueHandling HaNodeManagerIODataProviderBridgePrivate::getValueHandling(
            const NodeId & nodeId) {
        // get node properties
        const NodeProperties* nodeProps = dfltNodeProps;
        if (nodePropsMap != NULL) {
            for (std::vector<const NodeData*>::const_iterator i =
                    nodePropsMap->begin(); i != nodePropsMap->end(); i++) {
                if (nodeId.equals((*i)->getNodeId())) {
                    nodeProps = static_cast<const NodeProperties*> ((*i)->getData());
                    break;
                }
            }
        }
        if (nodeProps != NULL) {
            return nodeProps->getValueHandling();
        }
        std::ostringstream msg;
        msg << "Missing value handling mode from IO data provider for variable "
                << nodeId.toString();
        throw ExceptionDef(HaNodeManagerIODataProviderBridgeException, msg.str());
    }

} // namespace SASModelProviderNamespace
