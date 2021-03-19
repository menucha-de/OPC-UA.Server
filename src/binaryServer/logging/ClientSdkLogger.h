#ifndef BINARYSERVER_LOGGING_CLIENTSDKLOGGER_H
#define BINARYSERVER_LOGGING_CLIENTSDKLOGGER_H

#include <common/logging/Logger.h>

class ClientSdkLoggerPrivate;

class ClientSdkLogger : public CommonNamespace::Logger {
public:
    ClientSdkLogger(const char* name);
    ~ClientSdkLogger();

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
    virtual void println(const char* level, const char* loggerName, const char* msg,
            const char* msgPrefix = "", const char* msgSuffix = "");
private:
    ClientSdkLoggerPrivate* d;
};

#endif /* BINARYSERVER_LOGGING_CLIENTSDKLOGGER_H */

