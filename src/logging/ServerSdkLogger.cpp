#include "ServerSdkLogger.h"
#include <stdarg.h> // va_list
#include <stdio.h> // vasprintf
#include <malloc.h> // free
#include <srvtrace.h> // SrvT
#include <string>

class ServerSdkLoggerPrivate {
    friend class ServerSdkLogger;
private:
    static const int MODULE_NAME;
    static const int MAX_MSG_LEN;
    static const char LINE_DELIMITER[];

    std::string name;
};

const int ServerSdkLoggerPrivate::MODULE_NAME = 10;
// the SDK API limits a log entry to 1899 chars
const int ServerSdkLoggerPrivate::MAX_MSG_LEN = 1800;
const char ServerSdkLoggerPrivate::LINE_DELIMITER[] = "\n";

ServerSdkLogger::ServerSdkLogger(const char* name) {
    d = new ServerSdkLoggerPrivate();
    d->name = std::string(name);
}

ServerSdkLogger::~ServerSdkLogger() {
    delete d;
}

void ServerSdkLogger::error(const char* format, ...) {
    if (!isErrorEnabled()) {
        return;
    }
    char* buffer;
    va_list args;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
    while (bufferLine != NULL) {
        size_t bufferLineLength = strlen(bufferLine);
        if (ServerSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ServerSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println(ServerSdkLoggerPrivate::MODULE_NAME, "ERROR", d->name.c_str(), &bufferLine[i],
                        (j > ServerSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println(ServerSdkLoggerPrivate::MODULE_NAME, "ERROR", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ServerSdkLogger::warn(const char* format, ...) {
    if (!isWarnEnabled()) {
        return;
    }
    char* buffer;
    va_list args;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
    while (bufferLine != NULL) {
        size_t bufferLineLength = strlen(bufferLine);
        if (ServerSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ServerSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println(ServerSdkLoggerPrivate::MODULE_NAME, "WARN", d->name.c_str(), &bufferLine[i],
                        (j > ServerSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println(ServerSdkLoggerPrivate::MODULE_NAME, "WARN", d->name.c_str(),
                    bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ServerSdkLogger::info(const char* format, ...) {
    if (!isInfoEnabled()) {
        return;
    }
    char* buffer;
    va_list args;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
    while (bufferLine != NULL) {
        size_t bufferLineLength = strlen(bufferLine);
        if (ServerSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ServerSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println(ServerSdkLoggerPrivate::MODULE_NAME, "INFO", d->name.c_str(), &bufferLine[i],
                        (j > ServerSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println(ServerSdkLoggerPrivate::MODULE_NAME, "INFO", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ServerSdkLogger::debug(const char* format, ...) {
    if (!isDebugEnabled()) {
        return;
    }
    char* buffer;
    va_list args;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
    while (bufferLine != NULL) {
        size_t bufferLineLength = strlen(bufferLine);
        if (ServerSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ServerSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println(ServerSdkLoggerPrivate::MODULE_NAME, "DEBUG", d->name.c_str(), &bufferLine[i],
                        (j > ServerSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println(ServerSdkLoggerPrivate::MODULE_NAME, "DEBUG", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ServerSdkLogger::trace(const char* format, ...) {
    if (!isTraceEnabled()) {
        return;
    }
    char* buffer;
    va_list args;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
    while (bufferLine != NULL) {
        size_t bufferLineLength = strlen(bufferLine);
        if (ServerSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ServerSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println(ServerSdkLoggerPrivate::MODULE_NAME, "TRACE", d->name.c_str(), &bufferLine[i],
                        (j > ServerSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println(ServerSdkLoggerPrivate::MODULE_NAME, "TRACE", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

bool ServerSdkLogger::isErrorEnabled() {
    return SrvT::getTraceLevel() >= SrvT::Errors;
}

bool ServerSdkLogger::isWarnEnabled() {
    return SrvT::getTraceLevel() >= SrvT::Warning;
}

bool ServerSdkLogger::isInfoEnabled() {
    return SrvT::getTraceLevel() >= SrvT::Info;
}

bool ServerSdkLogger::isDebugEnabled() {
    return SrvT::getTraceLevel() >= SrvT::InterfaceCall; // incl. CTorDtor
}

bool ServerSdkLogger::isTraceEnabled() {
    return SrvT::getTraceLevel() >= SrvT::ProgramFlow; // incl. Data
}

void ServerSdkLogger::println(int module, const char* level, const char* loggerName, const char* msg,
        const char* msgPrefix, const char* msgSuffix) {
    switch (level[0]) {
        case 'E':
            SrvT::sError(module, "%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'W':
            SrvT::sWarning(module, "%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'I':
            SrvT::sInfo(module, "%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'D':
            SrvT::sIfCall(module, "%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'T':
            SrvT::sInOut(module, "%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
    }
}
