#include "BaSASModelProvider.h"
#include "BaNodeManager.h"

class BaSASModelProviderPrivate {
    friend class BaSASModelProvider;
private:
    IODataProviderNamespace::IODataProvider* ioDataProvider;
};

BaSASModelProvider::BaSASModelProvider() {
    d = new BaSASModelProviderPrivate();
}

BaSASModelProvider::~BaSASModelProvider() {
    delete d;
}

void BaSASModelProvider::open(std::string& confDir,
        IODataProviderNamespace::IODataProvider& ioDataProvider) {
    d->ioDataProvider = &ioDataProvider;
}

void BaSASModelProvider::close() {
}

std::vector<NodeManager*>* BaSASModelProvider::createNodeManagers() {
    std::vector<NodeManager*>* nodeManagers = new std::vector<NodeManager*>();
    nodeManagers->push_back(new BaNodeManager(*d->ioDataProvider));
    return nodeManagers;
}

std::vector<UaServerApplicationModule*>* BaSASModelProvider::createServerApplicationModules() {
    return NULL;
}
