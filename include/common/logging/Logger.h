#ifndef COMMON_LOGGING_LOGGER_H
#define COMMON_LOGGING_LOGGER_H

#include <string>

namespace CommonNamespace {

    /* An implementation of this class must be thread safe. */
    class Logger {
    public:
        Logger();
        virtual ~Logger();

        virtual void error(const char* format, ...) = 0;
        virtual void warn(const char* format, ...) = 0;
        virtual void info(const char* format, ...) = 0;
        virtual void debug(const char* format, ...) = 0;
        virtual void trace(const char* format, ...) = 0;

        virtual bool isErrorEnabled() = 0;
        virtual bool isWarnEnabled() = 0;
        virtual bool isInfoEnabled() = 0;
        virtual bool isDebugEnabled() = 0;
        virtual bool isTraceEnabled() = 0;
    };
} // namespace CommonNamespace
#endif /* COMMON_LOGGING_LOGGER_H */

