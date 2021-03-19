#include "HaXmlUaNodeFactoryManagerSet.h"

class HaXmlUaNodeFactoryManagerSetPrivate {
    friend class HaXmlUaNodeFactoryManagerSet;
private:
    std::vector<HaXmlUaNodeFactoryManager*> factoryManagers;
    std::vector<HaXmlUaNodeFactoryNamespace*> nsFactories;
};

HaXmlUaNodeFactoryManagerSet::HaXmlUaNodeFactoryManagerSet() {
    d = new HaXmlUaNodeFactoryManagerSetPrivate();
}

HaXmlUaNodeFactoryManagerSet::~HaXmlUaNodeFactoryManagerSet() {
    // delete registered namespace related factories
    for (std::vector<HaXmlUaNodeFactoryNamespace*>::iterator i = d->nsFactories.begin();
            i != d->nsFactories.end(); i++) {
        delete *i;
    }
    delete d;
}

void HaXmlUaNodeFactoryManagerSet::add(HaXmlUaNodeFactoryManager& factoryManager) {
    d->factoryManagers.push_back(&factoryManager);
    for (int i = 0; i < d->nsFactories.size(); i++) {
        factoryManager.addNamespace(d->nsFactories[i]);
    }
}

void HaXmlUaNodeFactoryManagerSet::addNamespace(HaXmlUaNodeFactoryNamespace& nsFactory) {
    d->nsFactories.push_back(&nsFactory);
    for (int i = 0; i < d->factoryManagers.size(); i++) {
        d->factoryManagers[i]->addNamespace(&nsFactory);
    }
}