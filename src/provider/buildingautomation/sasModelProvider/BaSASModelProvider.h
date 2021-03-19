#ifndef PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BASASMODELPROVIDER_H_
#define PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BASASMODELPROVIDER_H_

#include <sasModelProvider/SASModelProvider.h>
#include <nodemanager.h> // NodeManager
#include <uaserverapplicationmodule.h> // UaServerApplicationModule
#include <vector>

class BaSASModelProviderPrivate;

class BaSASModelProvider: public SASModelProviderNamespace::SASModelProvider {
public:
	BaSASModelProvider();
	virtual ~BaSASModelProvider();

	// interface SASModelProvider
	virtual void open(std::string& confDir,
			IODataProviderNamespace::IODataProvider& ioDataProvider);
	virtual void close();
	virtual std::vector<NodeManager*>* createNodeManagers();
	virtual std::vector<UaServerApplicationModule*>* createServerApplicationModules();
private:
    BaSASModelProviderPrivate* d;	
};

#endif /* PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_BASASMODELPROVIDER_H_ */
