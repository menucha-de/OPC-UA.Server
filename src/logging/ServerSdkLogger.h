#ifndef BINARYSERVER_LOGGING_SERVERSDKLOGGER_H
#define BINARYSERVER_LOGGING_SERVERSDKLOGGER_H

#include <common/logging/Logger.h>

class ServerSdkLoggerPrivate;

class ServerSdkLogger : public CommonNamespace::Logger {
public:
    ServerSdkLogger(const char* name);
    ~ServerSdkLogger();

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
    virtual void println(int module, const char* level, const char* loggerName, const char* msg,
            const char* msgPrefix = "", const char* msgSuffix = "");
private:
    ServerSdkLoggerPrivate* d;
};

#endif /* BINARYSERVER_LOGGING_SERVERSDKLOGGER_H */

