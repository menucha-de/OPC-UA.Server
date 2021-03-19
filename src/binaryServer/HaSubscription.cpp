#include "HaSubscription.h"
#include "HaSubscriptionException.h"
#include "NodeAttributes.h"
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <statuscode.h> // UaStatus
#include <uaarraytemplates.h> // UaDataNotifications, UaEventFieldLists
#include <uaclientsdk.h> // UaSubscriptionCallback
#include <uacontentfilter.h> // UaContentFilter
#include <uadatetime.h> // UaDateTime
#include <uaeventfilter.h> // UaEventFilter
#include <uaqualifiedname.h> // UaQualifiedName
#include <uadiagnosticinfos.h> // UaDiagnosticInfo
#include <uanodeid.h> // UaNodeIds
#include <uastring.h> // UaString
#include <uavariant.h> // UaVariant
#include <sstream> // std::ostringstream
#include <map>

using namespace CommonNamespace;
using namespace BinaryServerNamespace;

class HaSubscriptionPrivate : public UaClientSdk::UaSubscriptionCallback {
    friend class HaSubscription;
private:

    class MonitoredItem {
    public:
        OpcUa_UInt32 id;
        NodeAttributes* nodeAttr;
        std::vector<BinaryServerNamespace::EventField*>* eventFields;
    };

    // a new subscription is created directly after a subscription has expired
    // the subscription time out should be at least 3 * KEEP_ALIVE_INTERVAL (see Part 4, 5.13.2.2)
    static const int SUBSCRIPTION_TIMEOUT = 3600000; // in ms
    static const int KEEP_ALIVE_INTERVAL = 5000; // in ms

    Logger* log;
    HaSubscription* parent;

    UaClientSdk::UaSession* session;
    int sendReceiveTimeout; // in sec.
    OpcUa_Double publishingInterval; // in ms        

    HaSubscription::HaSubscriptionCallback* callback;
    int clientHandleCounter;

    Mutex* mutex;
    UaClientSdk::UaSubscription* subscription;
    // clientHandle -> monitoredItem
    std::map<OpcUa_UInt32, MonitoredItem*> monitoredItems;

    void createSubscription() /* throws HaSubscriptionException */;
    void deleteSubscription() /* throws HaSubscriptionException */;

    // interface UaSubscriptionCallback
    virtual void subscriptionStatusChanged(
            OpcUa_UInt32 clientSubscriptionHandle, const UaStatus& status);
    virtual void dataChange(OpcUa_UInt32 clientSubscriptionHandle,
            const UaDataNotifications& dataNotifications,
            const UaDiagnosticInfos& diagnosticInfos);
    virtual void newEvents(OpcUa_UInt32 clientSubscriptionHandle,
            UaEventFieldLists& eventFieldList);
};

HaSubscription::HaSubscriptionCallback::HaSubscriptionCallback() {

}

HaSubscription::HaSubscriptionCallback::~HaSubscriptionCallback() {
}

HaSubscription::HaSubscription(UaClientSdk::UaSession& session,
        int sendReceiveTimeout, int publishingInterval,
        HaSubscriptionCallback& callback) /* throws MutexException */ {
    d = new HaSubscriptionPrivate();
    d->log = LoggerFactory::getLogger("HaSubscription");
    d->parent = this;
    d->session = &session;
    d->sendReceiveTimeout = sendReceiveTimeout;
    d->publishingInterval = publishingInterval; // ms        
    d->callback = &callback;
    d->clientHandleCounter = 1;
    d->mutex = new Mutex(); // MutexException    
}

HaSubscription::~HaSubscription() /* throws HaSubscriptionException */ {
    removeAll(); // HaSubscriptionException
    delete d->mutex;
    delete d;
}

void HaSubscription::add(std::vector<NodeAttributes*>& nodeAttributes,
        std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>& eventFields)
/* throws HaSubscriptionException */ {
    if (nodeAttributes.size() == 0) {
        return;
    }

    MutexLock lock(*d->mutex);
    UaMonitoredItemCreateRequests itemsToCreate;
    itemsToCreate.create(nodeAttributes.size());
    HaSubscriptionException* exception = NULL;
    // clientHandle -> monitoredItem
    std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*> addedMonitoredItems;
    int itemsToCreateIndex = 0;
    // for each node
    for (OpcUa_UInt32 i = 0; i < nodeAttributes.size(); i++) {
        NodeAttributes& attr = *nodeAttributes[i];
        std::vector<BinaryServerNamespace::EventField*>* evFields = NULL;
        // if event type
        if (*attr.getNodeClass() == OpcUa_NodeClass_ObjectType) {
            evFields = eventFields.find(attr.getNodeId())->second;
            itemsToCreate[itemsToCreateIndex].ItemToMonitor.AttributeId = OpcUa_Attributes_EventNotifier;
            itemsToCreate[itemsToCreateIndex].ItemToMonitor.NodeId.Identifier.Numeric = OpcUaId_Server;
            itemsToCreate[itemsToCreateIndex].RequestedParameters.SamplingInterval = 0.0;
            // set filter for event type
            UaContentFilter* contentFilter = new UaContentFilter();
            UaContentFilterElement* contentFilterElement = new UaContentFilterElement();
            contentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
            UaLiteralOperand* operand = new UaLiteralOperand();            
            operand->setLiteralValue(attr.getNodeId());
            //operand->setLiteralValue(UaNodeId(1006, 4));
            contentFilterElement->setFilterOperand(0 /*index*/, operand, 1 /*arraySize*/);
            contentFilter->setContentFilterElement(0 /*index*/, contentFilterElement, 1 /*arraySize*/);
            UaEventFilter eventFilter;
            UaSimpleAttributeOperand selectElement;
            UaQualifiedName elements[evFields->size()];
            for (int i = 0; i < evFields->size(); i++) {
                elements[i] = *(*evFields)[i]->getQualifiedName();
            }
            for (int i = 0; i < evFields->size(); i++) {
                selectElement.setBrowsePathElement(0 /* index */, elements[i],
                        1 /* arraySize */);
                eventFilter.setSelectClauseElement(i, selectElement, evFields->size());
            }
            eventFilter.setWhereClause(contentFilter);
            eventFilter.detachFilter(itemsToCreate[itemsToCreateIndex].RequestedParameters.Filter);
        } else {
            itemsToCreate[itemsToCreateIndex].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
            attr.getNodeId().copyTo(&itemsToCreate[itemsToCreateIndex].ItemToMonitor.NodeId);
            itemsToCreate[itemsToCreateIndex].RequestedParameters.SamplingInterval = -1; // -1: use the publishing interval
        }
        OpcUa_UInt32 clientHandle = d->clientHandleCounter++;
        itemsToCreate[itemsToCreateIndex].RequestedParameters.ClientHandle = clientHandle;
        itemsToCreate[itemsToCreateIndex].RequestedParameters.QueueSize = 10;
        itemsToCreate[itemsToCreateIndex].RequestedParameters.DiscardOldest = OpcUa_True;
        itemsToCreate[itemsToCreateIndex].MonitoringMode = OpcUa_MonitoringMode_Reporting;
        itemsToCreateIndex++;
        // add nodeId to internal lists before adding the node as monitored item 
        // because the server may send dataChange events before the call is finished
        HaSubscriptionPrivate::MonitoredItem* monitoredItem = new HaSubscriptionPrivate::MonitoredItem();
        NodeAttributes* nodeAttrCopy = new NodeAttributes(attr);
        nodeAttrCopy->setValue(NULL);
        nodeAttrCopy->setException(NULL);
        monitoredItem->nodeAttr = nodeAttrCopy;
        std::vector<BinaryServerNamespace::EventField*>* eventFieldsCopy = NULL;
        if (evFields != NULL) {
            eventFieldsCopy = new std::vector<BinaryServerNamespace::EventField*>();
            for (int i = 0; i < evFields->size(); i++) {
                eventFieldsCopy->push_back(new BinaryServerNamespace::EventField(*(*evFields)[i]));
            }
        }
        monitoredItem->eventFields = eventFieldsCopy;
        d->monitoredItems[clientHandle] = monitoredItem;
        addedMonitoredItems[clientHandle] = monitoredItem;
    }
    if (itemsToCreateIndex > 0) {
        itemsToCreate.resize(itemsToCreateIndex);
        // if no subscription exists
        if (d->subscription == NULL) {
            // create a new subscription
            d->createSubscription(); // HaSubscriptionException
        }
        // add monitored items    
        UaClientSdk::ServiceSettings serviceSettings;
        serviceSettings.callTimeout = d->sendReceiveTimeout * 1000;
        UaMonitoredItemCreateResults results;
        d->log->debug("Adding %d monitored items to subscription %d", itemsToCreate.length(),
                d->subscription->subscriptionId());
        UaStatus result = d->subscription->createMonitoredItems(serviceSettings,
                OpcUa_TimestampsToReturn_Both, itemsToCreate, results);
        d->log->debug("Added monitored items");
        // if subscribing succeeded
        if (result.isGood()) {
            // for each result
            for (OpcUa_UInt32 i = 0; i < results.length(); i++) {
                UaStatusCode statusCode(results[i].StatusCode);
                OpcUa_UInt32 clientHandle = itemsToCreate[i].RequestedParameters.ClientHandle;
                HaSubscriptionPrivate::MonitoredItem& monitoredItem
                        = *d->monitoredItems[clientHandle];
                if (statusCode.isGood()) {
                    monitoredItem.id = results[i].MonitoredItemId;
                    addedMonitoredItems.erase(clientHandle);
                    if (d->log->isDebugEnabled()) {
                        d->log->debug("Added monitored item %s clientHandle=%d,revisedQueueSize=%d,revisedSamplingInterval=%f",
                                monitoredItem.nodeAttr->getNodeId().toXmlString().toUtf8(),
                                clientHandle, results[i].RevisedQueueSize,
                                results[i].RevisedSamplingInterval);
                    }
                } else if (exception == NULL) {
                    std::ostringstream msg;
                    msg << "Cannot add monitored item "
                            << monitoredItem.nodeAttr->getNodeId().toXmlString().toUtf8()
                            << " to subscription " << d->subscription->subscriptionId()
                            << ": " << statusCode.toString().toUtf8();
                    exception = new ExceptionDef(HaSubscriptionException, msg.str());
                }
            }
        } else if (exception == NULL) {
            std::ostringstream msg;
            msg << "Cannot add monitored items to subscription " << d->subscription->subscriptionId()
                    << ": " << result.toString().toUtf8();
            exception = new ExceptionDef(HaSubscriptionException, msg.str());
        }
    }
    if (exception != NULL) {
        // if a subscription exists
        if (addedMonitoredItems.size() > 0) {
            // remove nodeIds which could not be added for monitoring from internal lists
            for (std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*>::const_iterator it
                    = addedMonitoredItems.begin(); it != addedMonitoredItems.end(); it++) {
                OpcUa_UInt32 clientHandle = it->first;
                HaSubscriptionPrivate::MonitoredItem* monitoredItem
                        = d->monitoredItems[clientHandle];
                d->monitoredItems.erase(clientHandle);
                delete monitoredItem->nodeAttr;
                {
                    VectorScopeGuard<BinaryServerNamespace::EventField>
                            eventFieldsSG(monitoredItem->eventFields);
                }
                delete monitoredItem;
            }
            // if the subscription was created in this method call
            if (d->monitoredItems.size() == 0) {
                // delete the subscription
                d->deleteSubscription();
            }
        }
        ScopeGuard<HaSubscriptionException> exceptionSG(exception);
        throw *exception;
    }
}

void HaSubscription::remove(const std::vector<const UaNodeId*>& nodeIds)
/* throws HaSubscriptionException */ {
    if (nodeIds.size() == 0) {
        return;
    }

    MutexLock lock(*d->mutex);
    UaUInt32Array clientHandles;
    clientHandles.create(nodeIds.size());
    UaUInt32Array monitoredItemIds;
    monitoredItemIds.create(nodeIds.size());
    int monitoredItemIdsIndex = 0;
    // for each node
    for (std::vector<const UaNodeId*>::const_iterator i = nodeIds.begin(); i != nodeIds.end();
            i++) {
        const UaNodeId& nodeId = **i;
        // for each monitored item
        for (std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*>::iterator it
                = d->monitoredItems.begin(); it != d->monitoredItems.end(); it++) {
            OpcUa_UInt32 clientHandle = it->first;
            HaSubscriptionPrivate::MonitoredItem& monitoredItem = *it->second;
            // if nodeId matches
            if (nodeId == monitoredItem.nodeAttr->getNodeId()) {
                // add client handle to list
                clientHandles[monitoredItemIdsIndex] = clientHandle;
                monitoredItemIds[monitoredItemIdsIndex++] = monitoredItem.id;
                break;
            }
        }
    }
    if (monitoredItemIdsIndex == 0) {
        return;
    }

    // delete monitored items
    monitoredItemIds.resize(monitoredItemIdsIndex);
    UaClientSdk::ServiceSettings serviceSettings;
    serviceSettings.callTimeout = d->sendReceiveTimeout * 1000;
    UaStatusCodeArray results;
    d->log->debug("Removing %d monitored items from subscription %d", monitoredItemIds.length(),
            d->subscription->subscriptionId());
    UaStatus result = d->subscription->deleteMonitoredItems(serviceSettings, monitoredItemIds,
            results);
    d->log->debug("Removed monitored items");
    // if deleting succeeded
    if (result.isGood()) {
        HaSubscriptionException* exception = NULL;
        // for each result
        for (OpcUa_UInt32 i = 0; i < results.length(); i++) {
            UaStatusCode statusCode(results[i]);
            OpcUa_UInt32 clientHandle = clientHandles[i];
            std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*>::iterator it =
                    d->monitoredItems.find(clientHandle);
            // if a nodeId shall be removed twice (eg. overridden variables) the monitored item 
            // is removed for the first nodeId and cannot be removed a second time 
            if (it != d->monitoredItems.end()) {
                HaSubscriptionPrivate::MonitoredItem* monitoredItem = it->second;
                if (statusCode.isGood()) {
                    // remove entries for monitored item                                                
                    if (d->log->isDebugEnabled()) {
                        d->log->debug("Removed monitored item %s clientHandle=%d",
                                monitoredItem->nodeAttr->getNodeId().toXmlString().toUtf8(),
                                clientHandle);
                    }
                    d->monitoredItems.erase(clientHandle);
                    delete monitoredItem->nodeAttr;
                    {
                        VectorScopeGuard<BinaryServerNamespace::EventField>
                                eventFieldsSG(monitoredItem->eventFields);
                    }
                    delete monitoredItem;
                } else if (exception == NULL) {
                    std::ostringstream msg;
                    msg << "Cannot remove monitored item "
                            << monitoredItem->nodeAttr->getNodeId().toXmlString().toUtf8()
                            << " (clientHandle " << clientHandle
                            << ") from subscription " << d->subscription->subscriptionId()
                            << ": " << statusCode.toString().toUtf8();
                    exception = new ExceptionDef(HaSubscriptionException, msg.str());
                }
            }
        }
        if (exception != NULL) {
            ScopeGuard<HaSubscriptionException> exceptionSG(exception);
            throw *exception;
        }
    } else {
        std::ostringstream msg;
        msg << "Cannot remove monitored items from subscription "
                << d->subscription->subscriptionId() << ": " << result.toString().toUtf8();
        throw ExceptionDef(HaSubscriptionException, msg.str());
    }

    // if no monitored items exist
    if (d->monitoredItems.size() == 0) {
        // delete the subscription
        d->deleteSubscription();
    }
}

void HaSubscription::removeAll() /* throws HaSubscriptionException */ {
    MutexLock lock(*d->mutex);
    // get nodeIds of currently monitored items        
    std::vector<const UaNodeId*> nodeIds;
    for (std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*>::const_iterator it
            = d->monitoredItems.begin(); it != d->monitoredItems.end(); it++) {
        nodeIds.push_back(&it->second->nodeAttr->getNodeId());
    }
    // remove items
    remove(nodeIds); // HaSubscriptionException
}

void HaSubscriptionPrivate::createSubscription() /* throws HaSubscriptionException */ {
    UaClientSdk::ServiceSettings serviceSettings;
    serviceSettings.callTimeout = sendReceiveTimeout * 1000;

    UaClientSdk::SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = publishingInterval;
    subscriptionSettings.lifetimeCount =
            static_cast<OpcUa_UInt32> (SUBSCRIPTION_TIMEOUT / publishingInterval);
    subscriptionSettings.maxKeepAliveCount =
            static_cast<OpcUa_UInt32> (KEEP_ALIVE_INTERVAL / publishingInterval);

    UaStatus result = session->createSubscription(serviceSettings, this /* UaSubscriptionCallback*/,
            1 /* clientSubscriptionHandle */, subscriptionSettings,
            OpcUa_True /* publishingEnabled */, &subscription);
    if (!result.isGood()) {
        subscription = NULL;
        std::ostringstream msg;
        msg << "Cannot create a subscription: " << result.toString().toUtf8();
        throw ExceptionDef(HaSubscriptionException, msg.str());
    }

    if (log->isDebugEnabled()) {
        log->debug("Created subscription %d: revisedPublishingInterval=%f,revisedSubscriptionTimeout=%d,revisedKeepAliveInterval=%d\n",
                subscription->subscriptionId(), subscriptionSettings.publishingInterval,
                static_cast<OpcUa_UInt32> (
                subscriptionSettings.lifetimeCount * subscriptionSettings.publishingInterval),
                static_cast<OpcUa_UInt32> (
                subscriptionSettings.maxKeepAliveCount * subscriptionSettings.publishingInterval));
    }
}

void HaSubscriptionPrivate::deleteSubscription() /* throws HaSubscriptionException */ {
    if (subscription == NULL) {
        return;
    }

    OpcUa_UInt32 subscriptionId = subscription->subscriptionId();
    // delete the subscription
    UaClientSdk::ServiceSettings serviceSettings;
    serviceSettings.callTimeout = sendReceiveTimeout * 1000;
    UaStatus result = session->deleteSubscription(serviceSettings, &subscription);
    if (!result.isGood()) {
        std::ostringstream msg;
        msg << "Cannot delete subscription " << subscriptionId
                << ": " << result.toString().toUtf8();
        throw ExceptionDef(HaSubscriptionException, msg.str());
    }

    log->debug("Deleted subscription %d\n", subscriptionId);
    subscription = NULL;
}

void HaSubscriptionPrivate::subscriptionStatusChanged(
        OpcUa_UInt32 clientSubscriptionHandle, const UaStatus & status) {
    log->debug("Status changed for subscription %d: %s",
            subscription->subscriptionId(), status.toString().toUtf8());
    switch (status.code()) {
        case OpcUa_BadSubscriptionIdInvalid:
        {
            // get node attributes and event fields of currently monitored items
            // and remove the items from internal list
            std::vector<NodeAttributes*>* nodeAttributes = new std::vector<NodeAttributes*>();
            VectorScopeGuard<NodeAttributes> nodeAttributesSG(nodeAttributes);
            std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*> eventFields;
            MutexLock lock(*mutex);
            for (std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*>::const_iterator it
                    = monitoredItems.begin(); it != monitoredItems.end(); it++) {
                HaSubscriptionPrivate::MonitoredItem* monitoredItem = it->second;
                nodeAttributes->push_back(monitoredItem->nodeAttr);
                if (monitoredItem->eventFields != NULL) {
                    eventFields[monitoredItem->nodeAttr->getNodeId()] = monitoredItem->eventFields;
                }
                delete monitoredItem;
            }
            monitoredItems.clear();
            try {
                // clear the client side subscription object
                deleteSubscription(); // HaSubscriptionException                
                // add node attributes and event fields as new monitored items to a new subscription
                parent->add(*nodeAttributes, eventFields); // HaSubscriptionException
                // delete eventField structure
                for (std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>::const_iterator it
                        = eventFields.begin(); it != eventFields.end(); it++) {
                    VectorScopeGuard<BinaryServerNamespace::EventField> evFieldsSG(it->second);
                }
            } catch (Exception& e) {
                // delete eventField structure
                for (std::map<UaNodeId, std::vector<BinaryServerNamespace::EventField*>*>::const_iterator it
                        = eventFields.begin(); it != eventFields.end(); it++) {
                    VectorScopeGuard<BinaryServerNamespace::EventField> evFieldsSG(it->second);
                }
                // the error cannot be returned => log it
                std::string st;
                e.getStackTrace(st);
                log->error("Exception while creating new subscription: %s", st.c_str());
            }
            break;
        }
        default:
            break;
    }
}

void HaSubscriptionPrivate::dataChange(OpcUa_UInt32 clientSubscriptionHandle,
        const UaDataNotifications& dataNotifications, const UaDiagnosticInfos & diagnosticInfos) {
    MutexLock lock(*mutex);
    log->debug("Received %d data change notifications", dataNotifications.length());
    log->debug("DataChange Notifications ----------------------");
    // for each notification
    for (OpcUa_UInt32 i = 0; i < dataNotifications.length(); i++) {
        std::map<OpcUa_UInt32, HaSubscriptionPrivate::MonitoredItem*>::iterator it
                = monitoredItems.find(dataNotifications[i].ClientHandle);
        // if a subscription for the nodeId exists
        if (it != monitoredItems.end()) {
            std::vector<NodeAttributes*>* nodeAttributes = new std::vector<NodeAttributes*>();
            VectorScopeGuard<NodeAttributes> nodeAttributesSG(nodeAttributes);
            // maybe there are several notifications for one monitored item 
            // => the existing node attribute list cannot be used directly
            NodeAttributes* nodeAttrWithValues = new NodeAttributes(*it->second->nodeAttr);
            UaStatus status(dataNotifications[i].Value.StatusCode);
            if (status.isGood()) {
                // set value to internal object
                nodeAttrWithValues->setValue(new UaVariant(dataNotifications[i].Value.Value));
                if (log->isDebugEnabled()) {
                    log->debug("  %s %s value=%s",
                            UaDateTime(dataNotifications[i].Value.ServerTimestamp).toTimeString().toUtf8(),
                            nodeAttrWithValues->getNodeId().toXmlString().toUtf8(),
                            nodeAttrWithValues->getValue()->toFullString().toUtf8());
                }
            } else {
                std::ostringstream msg;
                msg << "Exception for " << nodeAttrWithValues->getNodeId().toXmlString().toUtf8()
                        << " found in data notification: " << status.toString().toUtf8();
                nodeAttrWithValues->setException(new ExceptionDef(HaSubscriptionException, msg.str()));
            }
            nodeAttributes->push_back(nodeAttrWithValues);
            try {
                // send notification    
                callback->dataChanged(*nodeAttributes); // Exception
            } catch (Exception& e) {
                // the error cannot be returned => log it
                std::string st;
                e.getStackTrace(st);
                log->error("Exception while processing notifications: %s", st.c_str());
            }
        }
    }
}

void HaSubscriptionPrivate::newEvents(OpcUa_UInt32 clientSubscriptionHandle,
        UaEventFieldLists & eventFieldList) {
    MutexLock lock(*mutex);
    log->debug("Received %d events", eventFieldList.length());
    log->debug("Events ----------------------------------------");
    // for each event
    for (OpcUa_UInt32 i = 0; i < eventFieldList.length(); i++) {
        HaSubscriptionPrivate::MonitoredItem& monitoredItem =
                *monitoredItems[eventFieldList[i].ClientHandle];
        if (log->isDebugEnabled()) {
            log->debug("  eventType=%s",
                    monitoredItem.nodeAttr->getNodeId().toXmlString().toUtf8());
        }
        std::vector<const Event*>* events = new std::vector<const Event*>();
        VectorScopeGuard<const Event> eventsSG(events);
        std::vector<BinaryServerNamespace::EventField*>* eventFieldsWithValues
                = new std::vector<BinaryServerNamespace::EventField*>();
        // for each event field
        for (int j = 0; j < monitoredItem.eventFields->size(); j++) {
            // maybe there are several events for one monitored item 
            // => the existing list of event fields cannot be used directly
            BinaryServerNamespace::EventField* eventFieldWithValue =
                    new BinaryServerNamespace::EventField(*(*monitoredItem.eventFields)[j]);
            if (eventFieldWithValue->getException() == NULL) {
                // set received value to event field
                UaVariant* value = new UaVariant(eventFieldList[i].EventFields[j]);
                if (log->isDebugEnabled()) {
                    log->debug("    sourceNode=%s,sourceName=%s,dataType=%s,value=%s",
                            eventFieldWithValue->getNodeId().toXmlString().toUtf8(),
                            UaString(eventFieldWithValue->getQualifiedName()->name()).toUtf8(),
                            eventFieldWithValue->getDataTypeId()->toXmlString().toUtf8(),
                            value->toFullString().toUtf8());
                }
                eventFieldWithValue->setValue(value);
            }
            eventFieldsWithValues->push_back(eventFieldWithValue);
        }
        // add an event to list
        events->push_back(new Event(*new UaNodeId(monitoredItem.nodeAttr->getNodeId()),
                *eventFieldsWithValues, true /*attachValues */));
        try {
            // send events
            callback->newEvents(*events); // Exception
        } catch (Exception& e) {
            // the error cannot be returned => log it
            std::string st;
            e.getStackTrace(st);
            log->error("Exception while processing events: %s", st.c_str());
        }
    }
}
