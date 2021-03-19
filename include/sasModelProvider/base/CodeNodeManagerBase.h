#ifndef SASMODELPROVIDER_BASE_CODENODEMANAGERBASE_H
#define SASMODELPROVIDER_BASE_CODENODEMANAGERBASE_H

#include "EventTypeRegistry.h"
#include "HaNodeManager.h"
#include "HaNodeManagerIODataProviderBridge.h"
#include <ioDataProvider/IODataProvider.h>
#include <iomanageruanode.h> // UaVariableArray
#include <methodmanager.h> // MethodManagerCallback
#include <nodemanagerbase.h> // NodeManagerBase
#include <opcuatypes.h> // ServiceContext
#include <session.h> // Session
#include <statuscode.h> // UaStatus
#include <uadatavalue.h> // UaDataValueArray
#include <uaarraytemplates.h> // PDataValueArray
#include <uadatavariablecache.h> // UaVariableCache
#include <uastring.h> // UaString

namespace SASModelProviderNamespace {

    class CodeNodeManagerBasePrivate;

    class CodeNodeManagerBase : public NodeManagerBase, public HaNodeManager {
    public:
        CodeNodeManagerBase(const UaString& sNamespaceUri,
                IODataProviderNamespace::IODataProvider& ioDataProvider,
                OpcUa_Boolean firesEvents = OpcUa_False);
        virtual ~CodeNodeManagerBase();

        // interface HaNodeManager
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
                TransactionType transactionType);
        virtual UaStatus beginCall(MethodManagerCallback* pCallback,
                const ServiceContext& serviceContext, OpcUa_UInt32 callbackHandle,
                MethodHandle* pMethodHandle, const UaVariantArray& inputArguments);
        virtual NodeManager& getNodeManagerRoot();
        virtual NodeManagerBase& getNodeManagerBase();
        virtual const std::vector<HaNodeManager*>* getAssociatedNodeManagers();
        virtual EventTypeRegistry& getEventTypeRegistry();
        virtual HaNodeManagerIODataProviderBridge& getIODataProviderBridge();
        virtual UaString getNameSpaceUri();
        virtual const UaString& getDefaultLocaleId() const;
        virtual void setVariable(UaVariable& variable,
                UaVariant& newValue) /* throws HaNodeManagerException */;                       
    private:
        CodeNodeManagerBase(const CodeNodeManagerBase&);
        CodeNodeManagerBase& operator=(const CodeNodeManagerBase&);

        CodeNodeManagerBasePrivate* d;
    };

} // namespace SASModelProviderNamespace
#endif // SASMODELPROVIDER_BASE_CODENODEMANAGERBASE_H
