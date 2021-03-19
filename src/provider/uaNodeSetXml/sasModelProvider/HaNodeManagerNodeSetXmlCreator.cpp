#include "HaNodeManagerNodeSetXmlCreator.h"
#include "HaNodeManagerNodeSetXml.h"

class HaNodeManagerNodeSetXmlCreatorPrivate {
    friend class HaNodeManagerNodeSetXmlCreator;
private:
    HaXmlUaNodeFactoryManagerSet* xmlUaNodeFactoryManagerSet;
    std::vector<HaNodeManager*>* nodeManagers;
    EventTypeRegistry* eventTypeRegistry;
    IODataProviderNamespace::IODataProvider* ioDataProvider;
};

HaNodeManagerNodeSetXmlCreator::HaNodeManagerNodeSetXmlCreator(
        std::vector<HaNodeManager*>& nodeManagers,
        EventTypeRegistry& eventTypeRegistry,
        HaXmlUaNodeFactoryManagerSet& uaNodeFactoryManagerSet,
        IODataProviderNamespace::IODataProvider& ioDataProvider) {
    d = new HaNodeManagerNodeSetXmlCreatorPrivate();
    d->nodeManagers = &nodeManagers;
    d->eventTypeRegistry = &eventTypeRegistry;
    d->xmlUaNodeFactoryManagerSet = &uaNodeFactoryManagerSet;
    d->ioDataProvider = &ioDataProvider;
}

HaNodeManagerNodeSetXmlCreator::~HaNodeManagerNodeSetXmlCreator() {
    delete d;
}

NodeManagerNodeSetXml* HaNodeManagerNodeSetXmlCreator::createNodeManager(
        const UaString& namespaceUri) {
    HaNodeManagerNodeSetXml* nm = new HaNodeManagerNodeSetXml(namespaceUri,
            *d->nodeManagers, *d->eventTypeRegistry, *d->xmlUaNodeFactoryManagerSet,
            *d->ioDataProvider);
    d->nodeManagers->push_back(nm);
    return nm;
}
