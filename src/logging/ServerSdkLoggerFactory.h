#ifndef BINARYSERVER_LOGGING_SERVERSDKLOGGERFACTORY_H
#define BINARYSERVER_LOGGING_SERVERSDKLOGGERFACTORY_H

#include <common/logging/ILoggerFactory.h>
#include <common/logging/Logger.h>

class ServerSdkLoggerFactoryPrivate;

class ServerSdkLoggerFactory : public CommonNamespace::ILoggerFactory {
public:
    ServerSdkLoggerFactory() /*throws MutexException*/;
    virtual ~ServerSdkLoggerFactory();

    // interface ILoggerFactory
    virtual CommonNamespace::Logger* getLogger(const char* name);
private:
    ServerSdkLoggerFactory(const ServerSdkLoggerFactory& orig);
    ServerSdkLoggerFactory& operator=(const ServerSdkLoggerFactory&);

    ServerSdkLoggerFactoryPrivate* d;
};

#endif /* BINARYSERVER_LOGGING_SERVERSDKLOGGERFACTORY_H */

