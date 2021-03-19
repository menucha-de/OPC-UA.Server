#ifndef NATIVE_JLOGGERFACTORY_H
#define NATIVE_JLOGGERFACTORY_H

#include <common/logging/ILoggerFactory.h>
#include <common/logging/Logger.h>
#include <jni.h>

class JLoggerFactoryPrivate;

class JLoggerFactory : public CommonNamespace::ILoggerFactory {
public:
	JLoggerFactory() /* throws MutexException */;
    virtual ~JLoggerFactory();

    // interface ILoggerFactory
    virtual CommonNamespace::Logger* getLogger(const char* name);
    virtual void setEnv(JNIEnv *env);
protected:
    // creates a new logger instance
    virtual CommonNamespace::Logger* createLogger(const char* name);
private:
    JLoggerFactory(const JLoggerFactory& orig);
    JLoggerFactory& operator=(const JLoggerFactory&);

    JLoggerFactoryPrivate* d;
    JNIEnv *env;
};

#endif /* NATIVE_JLOGGERFACTORY_H */

