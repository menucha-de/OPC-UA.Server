#ifndef SASMODELPROVIDER_SASMODELPROVIDER_H_
#define SASMODELPROVIDER_SASMODELPROVIDER_H_

#include <ioDataProvider/IODataProvider.h>
#include <nodemanager.h> // NodeManager
#include <uaserverapplication.h> // UaServerApplicationModule
#include <string>
#include <vector>

namespace SASModelProviderNamespace {

    class SASModelProvider {
    public:
        SASModelProvider();
        virtual ~SASModelProvider();

        // Opens the communication to the model provider.
        // If the model provider is already connected then an exception is thrown.
        // References to the parameter values must not be saved in the implementation.
        virtual void open(std::string& confDir,
                IODataProviderNamespace::IODataProvider& ioDataProvider) /* throws SASModelProviderException */ = 0;

        // Closes the communication to the model provider.
        // If the model provider is already disconnected then nothing is to be done.
        virtual void close() = 0;

        // Returns node manager implementations. If no node manager is created then
        // a server application module must be provided via "createServerApplicationModule".
        // The returned container and its components must be deleted by the caller. The components are
        // responsible for destroying their own sub structures.
        virtual std::vector<NodeManager*>* createNodeManagers() = 0;

        // Returns server application modules. If no server application module is created then
        // a node manager must be provided via "createNodeManager".
        // The returned container and its components must be deleted by the caller. The components are
        // responsible for destroying their own sub structures.
        virtual std::vector<UaServerApplicationModule*>* createServerApplicationModules() = 0;
    };

} /* namespace SASModelProviderNamespace */
#endif /* SASMODELPROVIDER_SASMODELPROVIDER_H_ */
