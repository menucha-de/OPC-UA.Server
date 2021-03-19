#ifndef PROVIDER_BINARY_IODATAPROVIDER_BINARYIODATAPROVIDER_H
#define PROVIDER_BINARY_IODATAPROVIDER_BINARYIODATAPROVIDER_H

#include <ioDataProvider/IODataProvider.h>

class BinaryIODataProviderPrivate;

class BinaryIODataProvider : public IODataProviderNamespace::IODataProvider {
public:
    BinaryIODataProvider(bool unitTest = false)/* throws MutexException */;
    virtual ~BinaryIODataProvider();

    // interface IODataProvider
    virtual void open(
            const std::string& confDir)/* throws IODataProviderException */;
    virtual void close();
    virtual const IODataProviderNamespace::NodeProperties* getDefaultNodeProperties(
            const std::string& namespaceUri,
            int namespaceIndex)/* throws IODataProviderException */;
    virtual std::vector<const IODataProviderNamespace::NodeData*>* getNodeProperties(
            const std::string& namespaceUri,
            int namespaceIndex)/* throws IODataProviderException */;
    virtual std::vector<IODataProviderNamespace::NodeData*>* read(
            const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds)/* throws IODataProviderException */;
    virtual void write(
            const std::vector<const IODataProviderNamespace::NodeData*>& nodeData,
            bool sendValueChangedEvents)/* throws IODataProviderException */;
    virtual std::vector<IODataProviderNamespace::MethodData*>* call(
            const std::vector<const IODataProviderNamespace::MethodData*>& methodData)/* throws IODataProviderException */;
    virtual std::vector<IODataProviderNamespace::NodeData*>* subscribe(
            const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds,
            IODataProviderNamespace::SubscriberCallback& callback)/* throws IODataProviderException */;
    virtual void unsubscribe(
            const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds)/* throws IODataProviderException */;
private:
    BinaryIODataProviderPrivate* d;
};

#endif /* PROVIDER_BINARY_IODATAPROVIDER_BINARYIODATAPROVIDER_H */
