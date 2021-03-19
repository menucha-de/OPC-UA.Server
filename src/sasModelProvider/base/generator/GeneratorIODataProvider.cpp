#include "GeneratorIODataProvider.h"
#include "DataGenerator.h"
#include "GeneratorException.h"
#include "../../../utilities/linux.h" // getTime
#include <common/Exception.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/MapScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <ioDataProvider/Event.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <uaargument.h> // UaArgument
#include <uanodeid.h> // UaNodeId
#include <uavariant.h> // UaVariant
#include <ctime>
#include <map>
#include <sstream> // std::ostringstream
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;
using namespace IODataProviderNamespace;

namespace SASModelProviderNamespace {

    class GeneratorIODataProviderPrivate {
        friend class GeneratorIODataProvider;
    private:
        static int EVENT_COUNT;
        static int EVENT_INTERVAL;

        NodeBrowser* nodeBrowser;
        ConverterUa2IO* converter;
        DataGenerator* dataGenerator;

        Mutex* mutex;
        std::map<UaNodeId, SubscriberCallback*> eventTypes;

        void fireEvents() /* throws GeneratorExceptionn */;
    };

    int GeneratorIODataProviderPrivate::EVENT_COUNT = 10;
    int GeneratorIODataProviderPrivate::EVENT_INTERVAL = 1; // in sec.

    GeneratorIODataProvider::GeneratorIODataProvider(NodeBrowser& nodeBrowser,
            ConverterUa2IO& converter) {
        d = new GeneratorIODataProviderPrivate();
        d->nodeBrowser = &nodeBrowser;
        d->converter = &converter;
        d->dataGenerator = new DataGenerator(nodeBrowser, converter);
        d->mutex = new Mutex(); // MutexException
    }

    GeneratorIODataProvider::~GeneratorIODataProvider() {
        delete d->mutex;
        delete d->dataGenerator;
        delete d;
    }

    std::vector<NodeData*>* GeneratorIODataProvider::read(const std::vector<const NodeId*>& nodeIds)
    /* throws ConversionException, GeneratorExceptionn */ {
        std::vector<NodeData*>* ret = new std::vector<NodeData*>();
        VectorScopeGuard<NodeData> retSG(ret);
        std::vector<UaVariable*> variables;
        try {
            for (int i = 0; i < nodeIds.size(); i++) {
                // get UaVariable for NodeId
                UaNodeId* uaNodeId = d->converter->convertIo2ua(*nodeIds[i]); // ConversionException
                ScopeGuard<UaNodeId> uaNodeIdSG(uaNodeId);
                variables.push_back(d->nodeBrowser->getVariable(*uaNodeId));
            }
            // generate data for variables
            std::vector<UaVariant*>* generatedData = d->dataGenerator->generate(variables); // ConversionException, GeneratorExceptionn
            VectorScopeGuard<UaVariant> generatedDataSG(generatedData);
            // for each variable
            for (int i = 0; i < generatedData->size(); i++) {
                UaVariant* uaValue = (*generatedData)[i];
                UaVariable& variable = *variables[i];
                // convert UaNodeId to NodeId                
                ScopeGuard<NodeId> nodeIdSG(
                        d->converter->convertUa2io(variable.nodeId())); // ConversionException
                // convert UaVariant to Variant
                Variant* v = NULL;
                if (uaValue != NULL) {
                    try {
                        v = d->converter->convertUa2io(*uaValue,
                                variable.dataType()); // ConversionException            
                    } catch (Exception& e) {
                        // invalid data structure (eg. due to missing mandatory structure fields)
                    }
                }
                // create NodeData
                ret->push_back(new NodeData(*nodeIdSG.detach(), v, true /* attachValues*/));
            }
            // release references to variables
            for (int i = 0; i < variables.size(); i++) {
                variables[i]->releaseReference();
            }
        } catch (Exception& e) {
            // release references to variables
            for (int i = 0; i < variables.size(); i++) {
                variables[i]->releaseReference();
            }
            throw;
        }
        return retSG.detach();
    }

    std::vector<MethodData*>* GeneratorIODataProvider::call(std::vector<const MethodData*>& methodDataList)
    /* throws ConversionException, GeneratorExceptionn */ {

        timespec ts;
        ts.tv_sec = d->EVENT_INTERVAL;
        ts.tv_nsec = 0;
        for (int i = 0; i < d->EVENT_COUNT; i++) {
            d->fireEvents();
            nanosleep(&ts, NULL /*remaining*/);
        }

        std::vector<MethodData*>* ret = new std::vector<MethodData*>();
        VectorScopeGuard<MethodData> retSG(ret);
        for (int i = 0; i < methodDataList.size(); i++) {
            const MethodData& methodData = *methodDataList[i];
            std::vector<const Variant*>* outputArgs = new std::vector<const Variant*>();
            ret->push_back(new MethodData(*new NodeId(methodData.getObjectNodeId()),
                    *new NodeId(methodData.getMethodNodeId()), *outputArgs,
                    true /* attachValues */));
            // get UaArgument list for methodId
            UaNodeId* nodeId = d->converter->convertIo2ua(methodData.getMethodNodeId()); // ConversionException
            ScopeGuard<UaNodeId> nodeIdSG(nodeId);
            UaMethod* method = d->nodeBrowser->getMethod(*nodeId);
            UaReferenceLists* methodReferences = method->getUaReferenceLists();
            UaVariable* argsVariable = static_cast<UaVariable*> (
                    methodReferences->getTargetNodeByBrowseName(
                    UaQualifiedName("OutputArguments", 0 /*nsIndex*/)));
            if (argsVariable == NULL) {
                // proceed with the next method
                continue;
            }
            UaVariant argsValue(*argsVariable->value(NULL /* session*/).value());
            UaExtensionObjectArray argsEOA;
            argsValue.toExtensionObjectArray(argsEOA);
            std::vector<UaArgument*>* args = new std::vector<UaArgument*>();
            VectorScopeGuard<UaArgument> argsSG(args);
            for (int j = 0; j < argsEOA.length(); j++) {
                args->push_back(new UaArgument(argsEOA[j]));
            }
            method->releaseReference();
            // generate data for arguments
            std::vector<UaVariant*>* generatedData = d->dataGenerator->generate(*args); // ConversionException, GeneratorExceptionn
            VectorScopeGuard<UaVariant> generatedDataSG(generatedData);
            // for each UaVariant
            for (int j = 0; j < generatedData->size(); j++) {
                UaVariant* uaValue = (*generatedData)[j];
                Variant* value = NULL;
                if (uaValue != NULL) {
                    // convert UaVariant to Variant
                    value = d->converter->convertUa2io(*uaValue,
                            (*args)[j]->getDataType()); // ConversionException
                }
                outputArgs->push_back(value);
            }
        }
        return retSG.detach();
    }

    std::vector<NodeData*>* GeneratorIODataProvider::subscribe(const std::vector<const NodeId*>& nodeIds,
            SubscriberCallback& callback)
    /* throws ConversionException, GeneratorExceptionn */ {
        std::vector<const NodeId*> variables;
        // for each node
        for (int i = 0; i < nodeIds.size(); i++) {
            const NodeId* nodeId = nodeIds[i];
            // convert NodeId to UaNodeId
            UaNodeId* uaNodeId = d->converter->convertIo2ua(*nodeId); // ConversionException
            ScopeGuard<UaNodeId> uaNodeIdSG(uaNodeId);
            // get node
            UaNode* node = d->nodeBrowser->getNode(*uaNodeId);
            if (node != NULL) {
                switch (node->nodeClass()) {
                    case OpcUa_NodeClass_Variable:
                        // add variable
                        variables.push_back(nodeId);
                        break;
                    case OpcUa_NodeClass_ObjectType:
                        // register event type
                        MutexLock lock(*d->mutex);
                        d->eventTypes[*uaNodeId] = &callback;
                        break;
                }
                node->releaseReference();
            } else {
                std::ostringstream msg;
                msg << "Unknown node: " << uaNodeId->toXmlString().toUtf8();
                throw ExceptionDef(GeneratorException, msg.str());
            }
        }
        // generate data for variables
        std::vector<NodeData*>* variablesData = read(variables); // ConversionException, GeneratorException
        std::vector<NodeData*>* ret = new std::vector<NodeData*>();
        // for each node
        for (int i = 0; i < nodeIds.size(); i++) {
            const NodeId& nodeId = *nodeIds[i];
            bool found = false;
            // for each variable data
            for (int j = 0; !found && j < variablesData->size(); j++) {
                NodeData* nodeData = (*variablesData)[j];
                // if the variable data belong to the node
                if (nodeData->getNodeId().equals(nodeId)) {
                    // add variable data for the node to the result list
                    ret->push_back(nodeData);
                    found = true;
                    break;
                }
            }
            // if event type
            if (!found) {
                // add NULL for the node to the result list
                ret->push_back(new NodeData(*new NodeId(nodeId), NULL /*data*/,
                        true /*attachValues*/));
            }
        }
        return ret;
    }

    void GeneratorIODataProviderPrivate::fireEvents() /* throws GeneratorExceptionn */ {
        MutexLock lock(*mutex);
        GeneratorException* exception = NULL;
        std::vector<UaObjectType*> objectTypes;
        try {
            // for each event type
            for (std::map<UaNodeId, SubscriberCallback*>::const_iterator it = eventTypes.begin();
                    it != eventTypes.end(); it++) {
                // get UaObjectType                
                objectTypes.push_back(nodeBrowser->getObjectType(it->first));
            }
            // generate data for event types
            std::vector<OpcUaEventData*>* generatedData =
                    dataGenerator->generate(objectTypes); // ConversionException, GeneratorException
            VectorScopeGuard<OpcUaEventData> generatedDataSG(generatedData);
            // for each event type
            for (int i = 0; i < generatedData->size(); i++) {
                OpcUaEventData* eventData = (*generatedData)[i];
                if (eventData != NULL) {
                    UaNodeId uaNodeId = objectTypes[i]->nodeId();
                    try {
                        // convert UaNodeId to NodeId
                        NodeId* nodeId = converter->convertUa2io(uaNodeId); // ConversionException
                        ScopeGuard<NodeId> nodeIdSG(nodeId);
                        // create node data with nodeId
                        std::vector<const NodeData*>* nodeData = new std::vector<const NodeData*>();
                        nodeData->push_back(new NodeData(*nodeId, eventData));
                        // create event with time stamp and node data
                        Event event(getTime(), *nodeData, true /*attachValues*/);
                        // fire event
                        eventTypes[uaNodeId]->valuesChanged(event); // SubscriberCallbackException                        
                    } catch (Exception& e) {
                        if (exception != NULL) {
                            std::ostringstream msg;
                            msg << "Cannot fire event for type " << uaNodeId.toXmlString().toUtf8();
                            exception = new ExceptionDef(GeneratorException, msg.str());
                            exception->setCause(&e);
                        }
                    }
                }
            }
            // release references to object types
            for (int i = 0; i < objectTypes.size(); i++) {
                objectTypes[i]->releaseReference();
            }
        } catch (Exception& e) {
            // release references to object types
            for (int i = 0; i < objectTypes.size(); i++) {
                objectTypes[i]->releaseReference();
            }
            if (exception != NULL) {
                exception = new ExceptionDef(GeneratorException, std::string("Cannot fire events"));
                exception->setCause(exception);
            }
        }
        if (exception != NULL) {
            ScopeGuard<GeneratorException> exceptionSG(exception);
            throw *exception;
        }
    }
} // namespace SASModelProviderNamespace
