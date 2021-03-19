#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYNAMESPACE_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYNAMESPACE_H_

#include <basenodes.h> // UaBase::Variable
#include <instancefactory.h> // XmlUaNodeFactoryNamespace
#include <methodmanager.h> // MethodManager
#include <nodemanager.h> // NodeManagerConfig
#include <uabasenodes.h> // UaVariable
#include <uamutex.h> // UaMutexRefCounted
#include <uavariant.h> // UaVariant

class HaXmlUaNodeFactoryNamespacePrivate;

class HaXmlUaNodeFactoryNamespace : public XmlUaNodeFactoryNamespace {
public:
    HaXmlUaNodeFactoryNamespace(OpcUa_UInt16 namespaceIndex,
            MethodManager& methodManager);
    virtual ~HaXmlUaNodeFactoryNamespace();

    // interface XmlUaNodeFactoryNamespace
    virtual UaVariable* createVariable(UaBase::Variable *pVariable,
            XmlUaNodeFactoryManager *pFactory, NodeManagerConfig* pNodeConfig,
            UaMutexRefCounted* pSharedMutex = NULL);
    virtual UaObject* createObject(UaBase::Object *pObject,
            XmlUaNodeFactoryManager *pFactory, NodeManagerConfig* pNodeConfig,
            UaMutexRefCounted* pSharedMutex = NULL);
    virtual UaVariant defaultValue(const UaNodeId &dataTypeId,
            OpcUa_Int32 valueRank) const;
private:
    HaXmlUaNodeFactoryNamespacePrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYNAMESPACE_H_ */
