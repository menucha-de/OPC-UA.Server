#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/ConsoleLogger.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <stdio.h> // fprintf
#include <map>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class ConsoleLoggerFactoryPrivate {
    friend class ConsoleLoggerFactory;
private:
    std::map<const char*, Logger*> loggers;
    Mutex* mutex;
};

ConsoleLoggerFactory::ConsoleLoggerFactory()/*throws MutexException */ {
    d = new ConsoleLoggerFactoryPrivate();
    d->mutex = new Mutex(); // MutexException
}

ConsoleLoggerFactory::~ConsoleLoggerFactory() {
    // delete loggers
    for (std::map<const char*, Logger*>::const_iterator it = d->loggers.begin();
            it != d->loggers.end(); it++) {
        delete it->second;
    }
    delete d->mutex;
    delete d;
}

Logger* ConsoleLoggerFactory::getLogger(const char* name) {
    MutexLock lock(*d->mutex);
    std::map<const char*, Logger*>::const_iterator it = d->loggers.find(name);
    if (it == d->loggers.end()) {
        Logger* logger = createLogger(name);
        d->loggers[name] = logger;
        return logger;
    }
    return it->second;
}

CommonNamespace::Logger* ConsoleLoggerFactory::createLogger(const char* name) {
    return new ConsoleLogger(name);
}