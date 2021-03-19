#ifndef IODATAPROVIDER_IODATAPROVIDER_H_
#define IODATAPROVIDER_IODATAPROVIDER_H_

#include "NodeData.h"
#include "MethodData.h"
#include "NodeProperties.h"
#include "SubscriberCallback.h"
#include <string>
#include <vector>
#include <jni.h>
#include <jni_md.h>
#include <sasModelProvider/base/NodeBrowser.h>


namespace IODataProviderNamespace {

    class IODataProvider {
    public:
        IODataProvider();
        virtual ~IODataProvider();

        // Opens the communication to the data provider.
        // If the data provider is already connected then an exception is thrown.
        // References to the parameter values must not be saved in the implementation.
        virtual void open(const std::string& confDir) /* throws IODataProviderException */ = 0;

        // Opens the JNI interface for Java usage
        virtual void open(JNIEnv *env, jobject properties, jobject dataProvider) /* throws IODataProviderException */ = 0;

        // Closes the communication to the data provider.
        // If the data provider is already disconnected then nothing is to be done.
        virtual void close() = 0;

        // Gets the default node properties for all nodes of a namespace.
        // Different properties are provided for single nodes by method "getNodeProperties".
        // References to the parameter values must not be saved in the implementation.
        // The returned object must be deleted by the caller. The object is
        // responsible for destroying its own sub structures.
        virtual const NodeProperties* getDefaultNodeProperties(const std::string& namespaceUri,
                int namespaceId) /* throws IODataProviderException */ = 0;

        // Gets node properties overriding the default ones provided by method
        // "getDefaultNodeProperties".
        // References to the parameter values must neither be saved in the implementation
        // nor be returned.
        // The returned container and its components must be deleted by the caller.
        // The components are responsible for destroying their own sub structures.
        virtual std::vector<const NodeData*>* getNodeProperties(
                const std::string& namespaceUri,
                int namespaceId) /* throws IODataProviderException */ = 0;

        // Gets data from the data provider.
        // References to the parameter values must neither be saved in the implementation
        // nor be returned.
        // The returned container and its components must be deleted by the caller.
        // The components are responsible for destroying their own sub structures.
        virtual std::vector<NodeData*>* read(
                const std::vector<const NodeId*>& nodeIds) = 0 /* throws IODataProviderException */;
        // Writes data to the data provider.
        // References to the parameter values must not be saved in the implementation.
        // The parameters are not saved internally.
        virtual void write(const std::vector<const NodeData*>& nodeData,
                bool sendValuesChangedEvents) /* throws IODataProviderException */ = 0;

        // Calls a method of the data provider.
        // References to the parameter values must neither be saved in the implementation
        // nor be returned.
        // The returned container and its components must be deleted by the caller.
        // The components are responsible for destroying their own sub structures.
        virtual std::vector<MethodData*>* call(
                const std::vector<const MethodData*>& methodData) /* throws IODataProviderException */ = 0;

        // Subscribes to variables or events and returns the current values of variables.
        // If a subscription for a variable or event already exists then an exception is thrown.
        // The caller is informed about changes and new events via the callback.
        // Method "unsubscribe" must be used to cancel the subscriptions.
        // References to the parameter values of "nodeIds" must not be saved in the implementation.
        // References to the parameter value "callback" can be saved in the implementation
        // until the relating nodes are unsubscribed.
        // The returned container and its components must be deleted by the caller.
        // The components are responsible for destroying their own sub structures.
        virtual std::vector<IODataProviderNamespace::NodeData*>* subscribe(
                const std::vector<const NodeId*>& nodeIds,
                SubscriberCallback& callback) /* throws IODataProviderException */ = 0;
        // Cancels subscriptions for variables or events.
        // If no subscription exists for a given variable of event then nothing is to be done.
        // References to the parameter values must not be saved in the implementation.
        virtual void unsubscribe(
                const std::vector<const NodeId*>& nodeIds) /* throws IODataProviderException */ = 0;

        virtual void notification(JNIEnv *env, int ns, jobject id, jobject value) = 0;
        virtual void event(JNIEnv *env, int eNs, jobject event, int pNs, jobject param, long timestamp, int severity, jstring msg, jobject value) = 0;

        virtual void setNodeBrowser(SASModelProviderNamespace::NodeBrowser* nodeBrowser) = 0;

    };

} /* namespace IODataProviderNamespace */

#endif /* IODATAPROVIDER_IODATAPROVIDER_H_ */
