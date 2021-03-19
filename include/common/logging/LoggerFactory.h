#ifndef COMMON_LOGGING_LOGGERFACTORY_H
#define COMMON_LOGGING_LOGGERFACTORY_H

#include "ILoggerFactory.h"
#include "Logger.h"

namespace CommonNamespace {

    class LoggerFactoryPrivate;

    /* Provides a wrapper for a logger factory implementation: Logger instances are provided
     * via a static method. 
     * An instance of this logger factory must be created before the static method is called 
     * the first time. If a further instance is created the related logger factory implementation
     * overrides the previous one until the instance is destroyed.
     */
    class LoggerFactory {
    public:
        LoggerFactory(ILoggerFactory& iloggerFactory, bool attachValues = false);
        virtual ~LoggerFactory();

        /* Gets a logger. The name identifies the logger.
         * The returned instance is deleted by the logger factory implementation
         * and must NOT be deleted by the caller.
         * The call of this method is thread safe.
         */
        static Logger* getLogger(const char* name);
    private:
        LoggerFactory(const LoggerFactory& orig);
        LoggerFactory& operator=(const LoggerFactory&);

        LoggerFactoryPrivate* d;
    };
} // namespace CommonNamespace
#endif /* COMMON_LOGGING_LOGGERFACTORY_H */

