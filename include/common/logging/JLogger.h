#ifndef NATIVE_JLOGGER_H
#define NATIVE_JLOGGER_H

#include <common/logging/Logger.h>
#include <jni.h>
#include <jni_md.h>

class JLoggerPrivate;

class JLogger : public CommonNamespace::Logger {
public:
	JLogger(const char* name, JNIEnv *env);
    ~JLogger();

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

    static int currentLevel;

protected:
    virtual void log(char *buffer, const char *method);

private:
    JLoggerPrivate* d;
    JavaVM *jvm;

};

#endif /* NATIVE_JLOGGER_H */
