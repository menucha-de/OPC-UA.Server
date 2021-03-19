#ifndef PROVIDER_BINARY_IODATAPROVIDER_JDATAPROVIDER_H
#define PROVIDER_BINARY_IODATAPROVIDER_JDATAPROVIDER_H

#include <ioDataProvider/IODataProvider.h>
#include <jni.h>
#include <jni_md.h>
#include <common/native2J/Native2J.h>
#include <uanodeid.h> // UaNodeId
#include <sasModelProvider/base/NodeBrowser.h>

class JDataProviderPrivate;

class JDataProvider : public IODataProviderNamespace::IODataProvider {
public:
	JDataProvider(bool unitTest = false)/* throws MutexException */;
    virtual ~JDataProvider();

    // interface IODataProvider
    virtual void open(
            const std::string& confDir)/* throws IODataProviderException */;
    // interface IODataProvider
    virtual void open(
    		JNIEnv *env, jobject properties, jobject dataProvider)/* throws IODataProviderException */;
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

    virtual void notification(JNIEnv *env, int ns, jobject id, jobject value);
    virtual void event(JNIEnv *env, int eNs, jobject event, int pNs, jobject param, long timestamp, int severity, jstring msg, jobject value);

    virtual void setNodeBrowser(SASModelProviderNamespace::NodeBrowser* nodeBrowser);

private:

    bool findFieldModel(const UaNodeId &start);
    UaNodeId getUaNode(ParamId *pId);
    std::string getParamId(UaNodeId nId);
    void updateModel(UaNodeId nId);


    JDataProviderPrivate* d;
    Native2J *native2j;
    jobject jDataProvider;
    JavaVM *jvm;
    SASModelProviderNamespace::NodeBrowser* nodeBrowser;
    std::map<std::string, std::map<std::string, ModelType> > fields;

};

#endif /* PROVIDER_BINARY_IODATAPROVIDER_JDATAPROVIDER_H */
