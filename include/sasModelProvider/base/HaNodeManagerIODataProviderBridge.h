#ifndef SASMODELPROVIDER_BASE_HANODEMANAGERIOBRIDGE_H
#define SASMODELPROVIDER_BASE_HANODEMANAGERIOBRIDGE_H

#include "HaNodeManager.h"
#include "IODataManager.h"
#include <ioDataProvider/IODataProvider.h>
#include <ioDataProvider/Scalar.h>
#include <iomanageruanode.h> // UaVariableArray
#include <methodmanager.h> // MethodManager
#include <session.h> // Session
#include <statuscode.h> // UaStatus
#include <uaarraytemplates.h> // PDataValueArray
#include <uabasenodes.h> // UaObjectType
#include <uadatavalue.h> // UaDataValueArray
#include <uanodeid.h> // UaNodeId
#include <uavariant.h> // UaVariant
#include <vector>

namespace SASModelProviderNamespace {

    class HaNodeManagerIODataProviderBridgePrivate;

    class HaNodeManagerIODataProviderBridge : public IODataManager,
    public MethodManager {
    public:
        HaNodeManagerIODataProviderBridge(HaNodeManager& nodeManager,
                IODataProviderNamespace::IODataProvider& ioDataProvider);
        virtual ~HaNodeManagerIODataProviderBridge();

        // interface IODataManager
        virtual UaStatus afterStartUp();
        virtual UaStatus beforeShutDown();
        virtual UaStatus readValues(const UaVariableArray &arrUaVariables,
                UaDataValueArray &arrDataValues);
        virtual UaStatus writeValues(const UaVariableArray &arrUaVariables,
                const PDataValueArray &arrpDataValues,
                UaStatusCodeArray &arrStatusCodes);
        virtual void afterSetAttributeValue(Session* pSession, UaNode* pNode,
                OpcUa_Int32 attributeId, const UaDataValue& dataValue);
        virtual void variableCacheMonitoringChanged(UaVariableCache* pVariable,
                IOManager::TransactionType transactionType);

        // interface MethodManager
        virtual UaStatus beginCall(MethodManagerCallback* pCallback,
                const ServiceContext& serviceContext, OpcUa_UInt32 callbackHandle,
                MethodHandle* pMethodHandle, const UaVariantArray& inputArguments);

        // Creates subscriptions for event types. Received events from the IO data provider
        // are processed by creating OPC UA events and firing them via
        // the node manager.
        virtual void updateValueHandling(const std::vector<UaObjectType*>& objectTypes)
        /* throws HaNodeManagerIODataProviderBridgeException */;
        // Updates the value handling of variables and creates subscriptions for variables
        // which shall be processed asynchronously. Received events from the IO data provider
        // are processed by updating the value cache of the server via the node manager.
        virtual void updateValueHandling(const std::vector<UaVariable*>& variables)
        /* throws HaNodeManagerIODataProviderBridgeException */;

        // Converts a NodeId to a UaNodeId.
        // The returned UaNodeId instance must be destroyed by the caller.
        virtual UaNodeId* convert(const IODataProviderNamespace::NodeId& nodeId) const;
        // Converts a UaNodeId to a NodeId.
        // The returned NodeId instance must be destroyed by the caller.
        virtual IODataProviderNamespace::NodeId* convert(const UaNodeId& nodeId) const;
        // Converts a Variant to a UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        virtual UaVariant* convert(const IODataProviderNamespace::Variant& value,
                const UaNodeId& dataTypeId); /* throws ConversionException */
        // Converts a UaVariant to a Variant.
        // The returned Variant instance must be destroyed by the caller.
        virtual IODataProviderNamespace::Variant* convert(const UaVariant& value,
                const UaNodeId& dataTypeId);
    private:
        HaNodeManagerIODataProviderBridge(const HaNodeManagerIODataProviderBridge&);
        HaNodeManagerIODataProviderBridge& operator=(
                const HaNodeManagerIODataProviderBridge&);

        HaNodeManagerIODataProviderBridgePrivate* d;
    };

} // namespace SASModelProviderNamespace
#endif // SASMODELPROVIDER_BASE_HANODEMANAGERIOBRIDGE_H
