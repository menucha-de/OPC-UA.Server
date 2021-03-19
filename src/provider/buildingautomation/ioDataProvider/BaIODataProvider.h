#ifndef PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BAIODATAPROVIDER_H_
#define PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BAIODATAPROVIDER_H_

#include <ioDataProvider/IODataProvider.h>
#include <ioDataProvider/MethodData.h>
#include <ioDataProvider/NodeData.h>
#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/NodeProperties.h>
#include <ioDataProvider/SubscriberCallback.h>
#include <vector>
#include <string>

class BaIODataProviderPrivate;

class BaIODataProvider : public IODataProviderNamespace::IODataProvider {
public:
    BaIODataProvider();
    virtual ~BaIODataProvider();

    // interface IODataProvider
    virtual void open(const std::string& confDir);
    virtual void close();
    virtual const IODataProviderNamespace::NodeProperties* getDefaultNodeProperties(
            const std::string& namespaceUri, int namespaceId);
    virtual std::vector<const IODataProviderNamespace::NodeData*>* getNodeProperties(
            const std::string& namespaceUri, int namespaceId);
    virtual std::vector<IODataProviderNamespace::NodeData*>* read(
            const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds);
    virtual void write(
            const std::vector<const IODataProviderNamespace::NodeData*>& nodeData,
            bool sendValueChangedEvents);
    virtual std::vector<IODataProviderNamespace::MethodData*>* call(
            const std::vector<const IODataProviderNamespace::MethodData*>& methodData);
    virtual std::vector<IODataProviderNamespace::NodeData*>* subscribe(
            const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds,
            IODataProviderNamespace::SubscriberCallback& callback);
    virtual void unsubscribe(
            const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds);
private:
    BaIODataProviderPrivate* d;
};

#endif /* PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BAIODATAPROVIDER_H_ */


