#include "ServerSdkLoggerFactory.h"
#include "ServerSdkLogger.h"
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <stdio.h> // fprintf
#include <map>

using namespace CommonNamespace;

class ServerSdkLoggerFactoryPrivate {
    friend class ServerSdkLoggerFactory;
private:
    std::map<const char*, Logger*> loggers;
    Mutex* mutex;
};

ServerSdkLoggerFactory::ServerSdkLoggerFactory() /*throws MutexException*/ {
    d = new ServerSdkLoggerFactoryPrivate();
    d->mutex = new Mutex(); // MutexException
}

ServerSdkLoggerFactory::~ServerSdkLoggerFactory() {
    // delete loggers
    for (std::map<const char*, Logger*>::const_iterator it = d->loggers.begin();
            it != d->loggers.end(); it++) {
        delete it->second;
    }
    delete d;
}

Logger* ServerSdkLoggerFactory::getLogger(const char* name) {
    MutexLock lock(*d->mutex);
    std::map<const char*, Logger*>::const_iterator it = d->loggers.find(name);
    if (it == d->loggers.end()) {
        Logger* logger = new ServerSdkLogger(name);
        d->loggers[name] = logger;
        return logger;
    }
    return it->second;
}

