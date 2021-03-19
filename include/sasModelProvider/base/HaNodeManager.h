#ifndef SASMODELPROVIDER_BASE_HANODEMANAGER_H_
#define SASMODELPROVIDER_BASE_HANODEMANAGER_H_

#include "EventTypeRegistry.h"
#include "IODataManager.h"
#include <uabasenodes.h> // UaObjectType
#include <methodmanager.h> // MethodManager
#include <nodemanager.h> // NodeManager
#include <nodemanagerbase.h> // NodeManagerBase
#include <uastructuredefinition.h> // UaStructureDefinition

namespace SASModelProviderNamespace {

    class HaNodeManagerIODataProviderBridge;

    // An HaNodeManager implementation must
    //   - extend NodeManagerBase and provide the instance via "getNodeManagerBase"
    //   - create an instance of EventTypeRegistry and provide it via "getEventTypeRegistry"
    //   - create an instance of HaNodeManagerIODataProviderBridge and provide it
    //     via "getIODataProviderBridge"
    //   - implement the IODataManager interface by overwriting the relating methods of
    //     the NodeManagerBase implementation and delegating them to the
    //     HaNodeManagerIODataProviderBridge instance
    //   - implement the MethodManager interface by delegating the relating methods to the
    //     the HaNodeManagerIODataProviderBridge instance
    //   - implement the other methods using the NodeManagerBase implementation
    //   - create OPC UA types and objects
    //   - register event types and its fields at EventTypeRegistry
    //   - update the value handling of OPC UA types and objects via HaNodeManagerIODataProviderBridge

    class HaNodeManager : public IODataManager, public MethodManager {
    public:
        HaNodeManager();
        virtual ~HaNodeManager();

        virtual NodeManager& getNodeManagerRoot() = 0;
        virtual NodeManagerBase& getNodeManagerBase() = 0;
        virtual const std::vector<HaNodeManager*>* getAssociatedNodeManagers() = 0;
        virtual EventTypeRegistry& getEventTypeRegistry() = 0;
        virtual HaNodeManagerIODataProviderBridge& getIODataProviderBridge() = 0;
        virtual UaString getNameSpaceUri() = 0;
        virtual const UaString& getDefaultLocaleId() const = 0;
        virtual void setVariable(UaVariable& variable,
                UaVariant& newValue) = 0 /* throws HaNodeManagerException */;
    };

} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_HANODEMANAGER_H_ */
