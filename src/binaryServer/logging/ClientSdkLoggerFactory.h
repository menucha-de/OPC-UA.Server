#ifndef BINARYSERVER_LOGGING_CLIENTSDKLOGGERFACTORY_H
#define BINARYSERVER_LOGGING_CLIENTSDKLOGGERFACTORY_H

#include <common/logging/ILoggerFactory.h>
#include <common/logging/Logger.h>

class ClientSdkLoggerFactoryPrivate;

class ClientSdkLoggerFactory : public CommonNamespace::ILoggerFactory {
public:
    ClientSdkLoggerFactory() /*throws MutexException*/;
    virtual ~ClientSdkLoggerFactory();

    // interface ILoggerFactory
    virtual CommonNamespace::Logger* getLogger(const char* name);
private:
    ClientSdkLoggerFactory(const ClientSdkLoggerFactory& orig);
    ClientSdkLoggerFactory& operator=(const ClientSdkLoggerFactory&);

    ClientSdkLoggerFactoryPrivate* d;
};

#endif /* BINARYSERVER_LOGGING_CLIENTSDKLOGGERFACTORY_H */

