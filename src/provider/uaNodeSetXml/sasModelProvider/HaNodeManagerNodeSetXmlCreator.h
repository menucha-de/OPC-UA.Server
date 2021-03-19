#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXMLCREATOR_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXMLCREATOR_H_

#include <ioDataProvider/IODataProvider.h>
#include <sasModelProvider/base/EventTypeRegistry.h>
#include <sasModelProvider/base/HaNodeManager.h>
#include "HaXmlUaNodeFactoryManagerSet.h"
#include <nodemanagernodesetxml.h> // NodeManagerNodeSetXmlCreator
#include <uastring.h> // UaString
#include <vector>

class HaNodeManagerNodeSetXmlCreatorPrivate;

class HaNodeManagerNodeSetXmlCreator : public NodeManagerNodeSetXmlCreator {
public:
    HaNodeManagerNodeSetXmlCreator(            
            std::vector<SASModelProviderNamespace::HaNodeManager*>& nodeManagers,
            SASModelProviderNamespace::EventTypeRegistry& eventTypeRegistry,
            HaXmlUaNodeFactoryManagerSet& uaNodeFactoryManagerSet,
            IODataProviderNamespace::IODataProvider& ioDataProvider);
    virtual ~HaNodeManagerNodeSetXmlCreator();

    // interface NodeManagerNodeSetXmlCreator
    virtual NodeManagerNodeSetXml* createNodeManager(
            const UaString& namespaceUri);
private:
    HaNodeManagerNodeSetXmlCreatorPrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXMLCREATOR_H_ */
