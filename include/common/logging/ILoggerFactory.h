#ifndef COMMON_LOGGING_ILOGGERFACTORY_H
#define COMMON_LOGGING_ILOGGERFACTORY_H

#include "Logger.h"

namespace CommonNamespace {

    class ILoggerFactory {
    public:
        ILoggerFactory();
        virtual ~ILoggerFactory();

        /* Gets a logger. The name identifies the logger.
         * The returned instance must be deleted by the logger factory implementation.
         * The call of this method must be thread safe.
         */
        virtual Logger* getLogger(const char* name) = 0;
    };
} // namespace CommonNamespace
#endif /* COMMON_LOGGING_LOGGERFACTORY_H */

