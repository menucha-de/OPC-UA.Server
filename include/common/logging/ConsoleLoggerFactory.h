#ifndef COMMON_LOGGING_CONSOLELOGGERFACTORY_H
#define COMMON_LOGGING_CONSOLELOGGERFACTORY_H

#include "ILoggerFactory.h"
#include "Logger.h"

class ConsoleLoggerFactoryPrivate;

class ConsoleLoggerFactory : public CommonNamespace::ILoggerFactory {
public:
    ConsoleLoggerFactory() /* throws MutexException */;
    virtual ~ConsoleLoggerFactory();

    // interface ILoggerFactory
    virtual CommonNamespace::Logger* getLogger(const char* name);
protected:
    // creates a new logger instance
    virtual CommonNamespace::Logger* createLogger(const char* name);
private:
    ConsoleLoggerFactory(const ConsoleLoggerFactory& orig);
    ConsoleLoggerFactory& operator=(const ConsoleLoggerFactory&);

    ConsoleLoggerFactoryPrivate* d;
};

#endif /* COMMON_LOGGING_CONSOLELOGGERFACTORY_H */

