#include "HaXmlUaNodeFactoryManager.h"

class HaXmlUaNodeFactoryManagerPrivate {
    friend class HaXmlUaNodeFactoryManager;
private:
};

HaXmlUaNodeFactoryManager::HaXmlUaNodeFactoryManager() {
    d = new HaXmlUaNodeFactoryManagerPrivate();
}

HaXmlUaNodeFactoryManager::~HaXmlUaNodeFactoryManager() {
    delete d;
}

UaMethod* HaXmlUaNodeFactoryManager::createMethod(UaBase::Method *pMethod,
        NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex) {
    return XmlUaNodeFactoryManager::createMethod(pMethod, pNodeConfig,
            pSharedMutex);
}

UaDataType* HaXmlUaNodeFactoryManager::createDataType(
        UaBase::DataType *pDataType, NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex) {
    return XmlUaNodeFactoryManager::createDataType(pDataType,
            pNodeConfig, pSharedMutex);
}

UaObjectType* HaXmlUaNodeFactoryManager::createObjectType(
        UaBase::ObjectType *pObjectType, NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex) {
    return XmlUaNodeFactoryManager::createObjectType(pObjectType, pNodeConfig,
            pSharedMutex);
}

UaVariableType* HaXmlUaNodeFactoryManager::createVariableType(
        UaBase::VariableType* pVariableType, NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex) {
    return XmlUaNodeFactoryManager::createVariableType(pVariableType,
            pNodeConfig, pSharedMutex);
}
