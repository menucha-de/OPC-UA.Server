#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXML_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXML_H_

#include <ioDataProvider/IODataProvider.h>
#include <sasModelProvider/base/HaNodeManager.h>
#include "HaXmlUaNodeFactoryManagerSet.h"
#include <basenodes.h> // UaBase::Variable
#include <instancefactory.h> // XmlUaNodeFactoryManager
#include <iomanageruanode.h> // UaVariableArray
#include <methodmanager.h> // MethodHandle
#include <nodemanager.h> // NodeManager
#include <nodemanagerbase.h> // NodeManagerBase
#include <nodemanagernodesetxml.h> // NodeManagerNodeSetXml
#include <opcuatypes.h> // ServiceContext
#include <statuscode.h> // UaStatus
#include <uaarraytemplates.h> // PDataValueArray
#include <uabasenodes.h> // UaObjectType
#include <uadatavalue.h> // UaDataValue
#include <uanodeid.h> // UaNodeId
#include <uastring.h> // UaString
#include <vector>

using namespace SASModelProviderNamespace;

class HaNodeManagerNodeSetXmlPrivate;

class HaNodeManagerNodeSetXml : public NodeManagerNodeSetXml, public HaNodeManager {
public:
    HaNodeManagerNodeSetXml(const UaString& namespaceUri,            
            const std::vector<HaNodeManager*>& associatedNodeManagers,
            EventTypeRegistry& eventTypeRegistry, 
            HaXmlUaNodeFactoryManagerSet& uaNodeFactoryManagerSet,
            IODataProviderNamespace::IODataProvider& ioDataProvider);
    virtual ~HaNodeManagerNodeSetXml();

    // interface NodeManagerNodeSetXml
    virtual void allNodesAndReferencesCreated();
    virtual void variableTypeCreated(UaVariableType* pNewNode, UaBase::VariableType *pVariableType);
    virtual void variableCreated(UaVariable* pNewNode,
            UaBase::Variable *pVariable);
    virtual void objectTypeCreated(UaObjectType* pNewNode,
            UaBase::ObjectType *pObjectType);
    virtual void objectCreated(UaObject* pNewNode, UaBase::Object *pObject);
    virtual void methodCreated(UaMethod* pNewNode, UaBase::Method *pMethod);
    virtual void dataTypeCreated(UaDataType* pNewNode, UaBase::DataType *pDataType);

    // interface HaNodeManager
    virtual UaStatus afterStartUp();
    virtual UaStatus beforeShutDown();
    virtual UaStatus readValues(const UaVariableArray &arrUaVariables,
            UaDataValueArray &arrDataValues);
    virtual UaStatus writeValues(const UaVariableArray &arrUaVariables,
            const PDataValueArray &arrpDataValues,
            UaStatusCodeArray &arrStatusCodes);
    virtual OpcUa_Boolean beforeSetAttributeValue(Session* pSession, UaNode* pNode,
            OpcUa_Int32 attributeId, const UaDataValue& dataValue, OpcUa_Boolean& checkWriteMask);
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
    HaNodeManagerNodeSetXmlPrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_HANODEMANAGERNODESETXML_H_ */
