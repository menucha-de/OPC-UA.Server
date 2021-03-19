#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYMANAGER_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYMANAGER_H_

#include <basenodes.h> // UaBase::ObjectType
#include <instancefactory.h> // XmlUaNodeFactoryManager
#include <nodemanager.h> // NodeManagerConfig
#include <uabasenodes.h> // UaMethod
#include <uamutex.h> // UaMutexRefCounted
#include <stddef.h> // NULL

class HaXmlUaNodeFactoryManagerPrivate;

class HaXmlUaNodeFactoryManager : public XmlUaNodeFactoryManager {
public:
    HaXmlUaNodeFactoryManager();
    virtual ~HaXmlUaNodeFactoryManager();

    virtual UaMethod* createMethod(UaBase::Method *pMethod,
            NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex =
            NULL);
    virtual UaDataType* createDataType(
            UaBase::DataType *pDataType,
            NodeManagerConfig* pNodeConfig,
            UaMutexRefCounted* pSharedMutex = NULL);
    virtual UaObjectType* createObjectType(UaBase::ObjectType *pObjectType,
            NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex =
            NULL);
    virtual UaVariableType* createVariableType(
            UaBase::VariableType *pVariableType, NodeManagerConfig* pNodeConfig,
            UaMutexRefCounted* pSharedMutex = NULL);
private:
    HaXmlUaNodeFactoryManagerPrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYMANAGER_H_ */
