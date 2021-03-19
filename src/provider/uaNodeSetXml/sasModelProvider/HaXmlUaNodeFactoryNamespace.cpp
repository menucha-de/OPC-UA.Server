#include "HaXmlUaNodeFactoryNamespace.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>

using namespace CommonNamespace;

class HaXmlUaNodeFactoryNamespacePrivate {
    friend class HaXmlUaNodeFactoryNamespace;
private:
    Logger* log;
    MethodManager* methodManager;
};

HaXmlUaNodeFactoryNamespace::HaXmlUaNodeFactoryNamespace(OpcUa_UInt16 namespaceIndex,
        MethodManager& methodManager)
: XmlUaNodeFactoryNamespace(namespaceIndex) {
    d = new HaXmlUaNodeFactoryNamespacePrivate();
    d->log = LoggerFactory::getLogger("HaXmlUaNodeFactoryNamespace");
    d->methodManager = &methodManager;
    d->log->info("Created XML UaNode factory for namespace with index %d", namespaceIndex);
}

HaXmlUaNodeFactoryNamespace::~HaXmlUaNodeFactoryNamespace() {
    delete d;
}

UaVariable* HaXmlUaNodeFactoryNamespace::createVariable(
        UaBase::Variable* pVariable, XmlUaNodeFactoryManager* pFactory,
        NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex) {
    // as long as no own variable types are created this method is not called
    return createGenericVariable(pVariable, pFactory, pNodeConfig, pSharedMutex);
}

UaObject* HaXmlUaNodeFactoryNamespace::createObject(UaBase::Object* pObject,
        XmlUaNodeFactoryManager* pFactory, NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex) {
    if (d->log->isInfoEnabled()) {
        d->log->info("Setting method manager for %s (%s)",
                pObject->browseName().toString().toUtf8(), pObject->nodeId().toXmlString().toUtf8());
    }
    OpcUa::BaseObjectTypeGeneric* ret = createGenericObject(pObject, pFactory,
            pNodeConfig, pSharedMutex);
    ret->setMethodManager(d->methodManager);
    return ret;
}

UaVariant HaXmlUaNodeFactoryNamespace::defaultValue(const UaNodeId& dataTypeId,
        OpcUa_Int32 valueRank) const {
    // as long as no own variable types are created this method is not called
    UaVariant ret;
    return ret;
}
