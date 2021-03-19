#include "EventTypeData.h"
#include <common/Exception.h>
#include <common/ScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <sasModelProvider/base/IODataProviderSubscriberCallback.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include <ioDataProvider/Event.h>
#include <ioDataProvider/NodeData.h>
#include <ioDataProvider/OpcUaEventData.h>
#include <ioDataProvider/Scalar.h>
#include <ioDataProvider/SubscriberCallbackException.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <uanodeid.h> // UaNodeId
#include <uastring.h> // UaString
#include <uavariant.h> // UaVariant
#include <sstream> // std::ostringstream
#include <vector>

using namespace CommonNamespace;
using namespace IODataProviderNamespace;

namespace SASModelProviderNamespace {

    class IODataProviderSubscriberCallbackPrivate {
        friend class IODataProviderSubscriberCallback;
    private:
        Logger* log;

        HaNodeManager* haNodeManager;
        NodeBrowser* nodeBrowser;

        // dateTime: milliseconds since 01.01.1970
        void processOpcUaEvent(long long time, const UaNodeId& eventTypeId,
                const OpcUaEventData& eventData) /* throws ConversionException, SubscriberCallbackException */;
    };

    IODataProviderSubscriberCallback::IODataProviderSubscriberCallback(
            HaNodeManager& haNodeManager) {
        d = new IODataProviderSubscriberCallbackPrivate();
        d->log = LoggerFactory::getLogger("IODataProviderSubscriberCallback");
        d->haNodeManager = &haNodeManager;
        d->nodeBrowser = new NodeBrowser(haNodeManager);
    }

    IODataProviderSubscriberCallback::~IODataProviderSubscriberCallback() {
        delete d->nodeBrowser;
        delete d;
    }

    void IODataProviderSubscriberCallback::valuesChanged(
            const Event& event) /* throws SubscriberCallbackException */ {
        HaNodeManagerIODataProviderBridge& nmioBridge =
                d->haNodeManager->getIODataProviderBridge();
        const std::vector<const NodeData*>& nodeDataList = event.getNodeData();
        SubscriberCallbackException* exception = NULL;
        for (int i = 0; i < nodeDataList.size(); i++) {
            const NodeData& nodeData = *nodeDataList[i];
            try {                
                // convert NodeId to UaNodeId
            	//d->log->error("Convert-> %s", nodeData.getNodeId().toString().c_str());
                UaNodeId* nodeId = nmioBridge.convert(nodeData.getNodeId()); // ConversionException
                ScopeGuard<UaNodeId> nodeIdSG(nodeId);
                // if OPC UA event
                if (nodeData.getData() != NULL && nodeData.getData()->getVariantType()
                        == Variant::OPC_UA_EVENT_DATA) {
                    // process the event
                    d->processOpcUaEvent(event.getDateTime(), *nodeId,
                            *static_cast<const OpcUaEventData*> (nodeData.getData())); // ConversionException, SubscriberCallbackException
                } else {
                	//d->log->error("NodeBrowser-> %s", nodeId->toString().toUtf8());
                    UaVariable* variable = d->nodeBrowser->getVariable(*nodeId);
                    if (variable == NULL) {
                    	return;
//                        throw ExceptionDef(SubscriberCallbackException,
//                                std::string("Unknown variable ")
//                                .append(" received"));
                    }
                    //d->log->error("<-NodeBrowser %s", nodeData.getNodeId().toString().c_str());
                    try {
                        // convert Variant to UaVariant
                        UaVariant* value = nodeData.getData() == NULL ? new UaVariant()
                                : nmioBridge.convert(*nodeData.getData(),
                                variable->dataType()); // ConversionException                                  
                        ScopeGuard<UaVariant> valueSG(value);
                        // update variable
                        d->haNodeManager->setVariable(*variable, *value); // HaNodeManagerException
                        variable->releaseReference();
                    } catch (Exception& e) {
                        variable->releaseReference();
                        throw;
                    }
                }
            } catch (Exception& e) {
                if (exception == NULL) {
                    exception = new ExceptionDef(SubscriberCallbackException,
                            std::string("Processing of event failed"));
                    d->log->info(e.getMessage().c_str());
                    exception->setCause(&e);
                }
            }
        }
        if (exception != NULL) {
        	return;
        }
    }

    void IODataProviderSubscriberCallbackPrivate::processOpcUaEvent(long long time,
            const UaNodeId& eventTypeId, const OpcUaEventData& eventData)
    /* throws ConversionException, SubscriberCallbackException */ {
        HaNodeManagerIODataProviderBridge& nmioBridge = haNodeManager->getIODataProviderBridge();
        EventTypeData uaEvent(*eventTypeId, haNodeManager->getEventTypeRegistry());
        // SourceNode
        // convert NodeId to UaNodeId
        UaNodeId* sourceNodeId = nmioBridge.convert(
                eventData.getSourceNodeId()); // ConversionException
        ScopeGuard<UaNodeId> sourceNodeIdSG(sourceNodeId);
        uaEvent.setSourceNode(*sourceNodeId);
        // SourceName
        UaNode* sourceNode = nodeBrowser->getNode(*sourceNodeId);
        if (sourceNode == NULL) {
            throw ExceptionDef(SubscriberCallbackException,
                    std::string("Unknown source node ").append(sourceNodeId->toXmlString().toUtf8())
                    .append(" for event type ").append(eventTypeId.toXmlString().toUtf8())
                    .append(" received"));
        }
        uaEvent.setSourceName(sourceNode->browseName().toString());
        sourceNode->releaseReference();
        uaEvent.setMessage(
                UaLocalizedText("en",
                UaString(eventData.getMessage().c_str())));
        uaEvent.setSeverity(eventData.getSeverity());
        // set time stamps (in 100 ns) and unique EventId
        // 01.01.1601 - 01.01.1970: 11644473600 seconds        
        uaEvent.prepareNewEvent(
                UaDateTime(time * 10000 + 116444736000000000),
                UaDateTime::now() /* receiveTime */,
                UaByteString() /* userEventId */);
        // set field data
        const std::vector<const NodeData*>& fieldData = eventData.getFieldData();
        std::map<UaNodeId*, UaVariant*> addedFieldData;
        try {
            for (size_t i = 0; i < fieldData.size(); i++) {
                const NodeData& nodeData = *fieldData[i];
                // convert NodeId to UaNodeId
                UaNodeId* fieldNodeId = nmioBridge.convert(nodeData.getNodeId()); // ConversionException
                ScopeGuard<UaNodeId> fieldNodeIdSG(fieldNodeId);
                UaVariable* variable = nodeBrowser->getVariable(*fieldNodeId);
                if (variable == NULL) {
                    throw ExceptionDef(SubscriberCallbackException,
                            std::string("Unknown event field ")
                            .append(fieldNodeId->toXmlString().toUtf8()).append(" received"));
                }
                try {
                    // convert Variant to UaVariant
                    UaVariant* fieldValue = nodeData.getData() == NULL ? new UaVariant()
                            : nmioBridge.convert(*nodeData.getData(),
                            variable->dataType()); // ConversionException                            
                    // add field data to event
                    uaEvent.addFieldData(*fieldNodeId, *fieldValue);
                    addedFieldData[fieldNodeIdSG.detach()] = fieldValue;
                    variable->releaseReference();
                } catch (Exception& e) {
                    variable->releaseReference();
                    throw;
                }
            }
            // fire the event
            haNodeManager->getNodeManagerBase().fireEvent(&uaEvent);

            if (log->isInfoEnabled()) {
                log->info("Fired event %s %s", eventTypeId.toXmlString().toUtf8(),
                        uaEvent.getMessage().toString().toUtf8());
            }
            // for each added field data
            for (std::map<UaNodeId*, UaVariant*>::iterator i =
                    addedFieldData.begin(); i != addedFieldData.end(); i++) {
                UaNodeId* fieldNodeId = (*i).first;
                UaVariant* fieldValue = (*i).second;
                if (log->isDebugEnabled()) {
                    log->debug(" field nodeId=%s,value=%s", fieldNodeId->toXmlString().toUtf8(),
                            fieldValue->toString().toUtf8());
                }
                // delete field data
                delete fieldNodeId;
                delete fieldValue;
            }
        } catch (Exception& e) {
            // delete added field data
            for (std::map<UaNodeId*, UaVariant*>::iterator i =
                    addedFieldData.begin(); i != addedFieldData.end(); i++) {
                delete (*i).first;
                delete (*i).second;
            }
            throw;
        }
    }

} // namespace SASModelProviderNamespace
