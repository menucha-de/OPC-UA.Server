#ifndef PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BANODEMANAGER_H
#define PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BANODEMANAGER_H

#include <sasModelProvider/base/CodeNodeManagerBase.h>

class BaNodeManagerPrivate;

class BaNodeManager : public SASModelProviderNamespace::CodeNodeManagerBase {
public:

    class EventNotifierFolder : public UaFolder {
    public:

        EventNotifierFolder(const UaString& name, const UaNodeId& nodeId, const UaString& defaultLocaleId)
        : UaFolder(name, nodeId, defaultLocaleId) {
        }

        virtual OpcUa_Byte eventNotifier() const {
            return Ua_EventNotifier_SubscribeToEvents;
        }
    };

    BaNodeManager(IODataProviderNamespace::IODataProvider& ioDataProvider);
    virtual ~BaNodeManager();

    // interface CodeNodeManagerBase
    virtual UaStatus afterStartUp();
private:
    UaStatus createTypeNodes();
    UaStatus createObjectNodes();
    
    BaNodeManagerPrivate* d;
};

#endif // PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BANODEMANAGER_H
