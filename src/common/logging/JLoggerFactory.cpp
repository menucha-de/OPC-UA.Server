#include <common/logging/JLoggerFactory.h>
#include <common/logging/JLogger.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <stdio.h> // fprintf
#include <map>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class JLoggerFactoryPrivate {
    friend class JLoggerFactory;
private:
    std::map<const char*, Logger*> loggers;
    Mutex* mutex;
};

JLoggerFactory::JLoggerFactory()/*throws MutexException */ {
    d = new JLoggerFactoryPrivate();
    d->mutex = new Mutex(); // MutexException
    env = NULL;
}

JLoggerFactory::~JLoggerFactory() {
    // delete loggers
    for (std::map<const char*, Logger*>::const_iterator it = d->loggers.begin();
            it != d->loggers.end(); it++) {
        delete it->second;
    }
    delete d->mutex;
    delete d;
}

Logger* JLoggerFactory::getLogger(const char* name) {
    MutexLock lock(*d->mutex);
    std::map<const char*, Logger*>::const_iterator it = d->loggers.find(name);
    if (it == d->loggers.end()) {
        Logger* logger = createLogger(name);
        d->loggers[name] = logger;
        return logger;
    }
    return it->second;
}

CommonNamespace::Logger* JLoggerFactory::createLogger(const char* name) {
    return new JLogger(name, env);
}

void JLoggerFactory::setEnv(JNIEnv *env) {
	this->env = env;
}


