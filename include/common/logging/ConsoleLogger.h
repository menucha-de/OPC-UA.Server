#ifndef COMMON_LOGGING_CONSOLELOGGER_H
#define COMMON_LOGGING_CONSOLELOGGER_H

#include <common/logging/Logger.h>
#include <stdio.h> // FILE

class ConsoleLoggerPrivate;

class ConsoleLogger : public CommonNamespace::Logger {
public:
    ConsoleLogger(const char* name);
    ~ConsoleLogger();

    // interface Logger
    virtual void error(const char* format, ...);
    virtual void warn(const char* format, ...);
    virtual void info(const char* format, ...);
    virtual void debug(const char* format, ...);
    virtual void trace(const char* format, ...);
    virtual bool isErrorEnabled();
    virtual bool isWarnEnabled();
    virtual bool isInfoEnabled();
    virtual bool isDebugEnabled();
    virtual bool isTraceEnabled();

protected:
    virtual void println(FILE *outputStream, const char* timeStamp, const char* level, const char* loggerName,
            const char* msg);

private:
    ConsoleLoggerPrivate* d;
};

#endif /* COMMON_LOGGING_CONSOLELOGGER_H */

