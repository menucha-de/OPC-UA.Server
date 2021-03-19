#ifndef PROVIDER_UANODESETXML_SASMODELPROVIDER_UANODESETXMLSASMODELPROVIDER_H_
#define PROVIDER_UANODESETXML_SASMODELPROVIDER_UANODESETXMLSASMODELPROVIDER_H_

#include <ioDataProvider/IODataProvider.h>
#include <sasModelProvider/SASModelProvider.h>
#include <nodemanager.h> // NodeManager
#include <uaserverapplication.h> // UaServerApplicationModule
#include <string>
#include <vector>

class UaNodeSetXMLSASModelProviderPrivate;

class UaNodeSetXMLSASModelProvider : public SASModelProviderNamespace::SASModelProvider {
public:
    UaNodeSetXMLSASModelProvider();
    virtual ~UaNodeSetXMLSASModelProvider();

    // interface SASModelProvider
    virtual void open(std::string& confDir,
            IODataProviderNamespace::IODataProvider& ioDataProvider) /* throws SASModelProviderException */;
    virtual void close();
    virtual std::vector<NodeManager*>* createNodeManagers();
    virtual std::vector<UaServerApplicationModule*>* createServerApplicationModules();
private:
    UaNodeSetXMLSASModelProviderPrivate* d;
};

#endif /* PROVIDER_UANODESETXML_SASMODELPROVIDER_UANODESETXMLSASMODELPROVIDER_H_ */
