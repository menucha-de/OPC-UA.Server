#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYMANAGERSET_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYMANAGERSET_H_

#include "HaXmlUaNodeFactoryManager.h"
#include "HaXmlUaNodeFactoryNamespace.h"

class HaXmlUaNodeFactoryManagerSetPrivate;

class HaXmlUaNodeFactoryManagerSet {
public:
    HaXmlUaNodeFactoryManagerSet();
    virtual ~HaXmlUaNodeFactoryManagerSet();

    // Adds a UaNode factory manager which was created for a UaNodeSetXmlParserUaNode instance. 
    // Each added namespace related factory is added to the factory manager.
    // The factory manager instance is destroyed by the UaNodeSetXmlParserUaNode instance.
    void add(HaXmlUaNodeFactoryManager& factoryManager);
    // Adds a namespace related UaNode factory to all added factory managers.
    // The namespace related factory is destroyed by the destructor of this factory manager set
    // (the Sdk does not provide a method to remove a namespace related factory from a 
    // factory manager).
    void addNamespace(HaXmlUaNodeFactoryNamespace& nsFactory);
private:
    HaXmlUaNodeFactoryManagerSetPrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_HAXMLUANODEFACTORYMANAGERSET_H_ */
