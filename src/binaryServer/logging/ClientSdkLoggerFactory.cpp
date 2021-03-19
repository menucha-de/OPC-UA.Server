#include "ClientSdkLoggerFactory.h"
#include "ClientSdkLogger.h"
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <stdio.h> // fprintf
#include <map>

using namespace CommonNamespace;

class ClientSdkLoggerFactoryPrivate {
    friend class ClientSdkLoggerFactory;
private:
    std::map<const char*, Logger*> loggers;
    Mutex* mutex;
};

ClientSdkLoggerFactory::ClientSdkLoggerFactory() /*throws MutexException*/ {
    d = new ClientSdkLoggerFactoryPrivate();
    d->mutex = new Mutex(); // MutexException
}

ClientSdkLoggerFactory::~ClientSdkLoggerFactory() {
    // delete loggers
    for (std::map<const char*, Logger*>::const_iterator it = d->loggers.begin();
            it != d->loggers.end(); it++) {
        delete it->second;
    }
    delete d;
}

Logger* ClientSdkLoggerFactory::getLogger(const char* name) {
    MutexLock lock(*d->mutex);
    std::map<const char*, Logger*>::const_iterator it = d->loggers.find(name);
    if (it == d->loggers.end()) {
        Logger* logger = new ClientSdkLogger(name);
        d->loggers[name] = logger;
        return logger;
    }
    return it->second;
}

