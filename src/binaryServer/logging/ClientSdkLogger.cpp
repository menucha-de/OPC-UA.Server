#include "ClientSdkLogger.h"
#include <libtrace.h> // LibT
#include <stdarg.h> // va_list
#include <stdio.h> // vasprintf
#include <malloc.h> // free
#include <string>

class ClientSdkLoggerPrivate {
    friend class ClientSdkLogger;
private:
    static const int MAX_MSG_LEN;
    static const char LINE_DELIMITER[];
    std::string name;
};

// the SDK API limits a log entry to 1899 chars
const int ClientSdkLoggerPrivate::MAX_MSG_LEN = 1800;
const char ClientSdkLoggerPrivate::LINE_DELIMITER[] = "\n";

ClientSdkLogger::ClientSdkLogger(const char* name) {
    d = new ClientSdkLoggerPrivate();
    d->name = std::string(name);
}

ClientSdkLogger::~ClientSdkLogger() {
    delete d;
}

void ClientSdkLogger::error(const char* format, ...) {
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
        if (ClientSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ClientSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println("ERROR", d->name.c_str(), &bufferLine[i],
                        (j > ClientSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println("ERROR", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ClientSdkLogger::warn(const char* format, ...) {
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
        if (ClientSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ClientSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println("WARN", d->name.c_str(), &bufferLine[i],
                        (j > ClientSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println("WARN", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ClientSdkLogger::info(const char* format, ...) {
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
        if (ClientSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ClientSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println("INFO", d->name.c_str(), &bufferLine[i],
                        (j > ClientSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println("INFO", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ClientSdkLogger::debug(const char* format, ...) {
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
        if (ClientSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ClientSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println("DEBUG", d->name.c_str(), &bufferLine[i],
                        (j > ClientSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println("DEBUG", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

void ClientSdkLogger::trace(const char* format, ...) {
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
        if (ClientSdkLoggerPrivate::MAX_MSG_LEN < bufferLineLength) {
            size_t i = 0;
            while (i < bufferLineLength) {
                int j = i + ClientSdkLoggerPrivate::MAX_MSG_LEN;
                if (j > bufferLineLength) {
                    j = bufferLineLength;
                }
                char ch = bufferLine[j];
                bufferLine[j] = 0;
                println("TRACE", d->name.c_str(), &bufferLine[i],
                        (j > ClientSdkLoggerPrivate::MAX_MSG_LEN ? "..." : ""),
                        (j < bufferLineLength ? "..." : ""));
                bufferLine[j] = ch;
                i = j;
            }
        } else {
            println("TRACE", d->name.c_str(), bufferLine);
        }
        bufferLine = strtok(NULL, d->LINE_DELIMITER);
    }

    free(buffer);
}

bool ClientSdkLogger::isErrorEnabled() {

    return LibT::getTraceLevel() >= UaTrace::Errors;
}

bool ClientSdkLogger::isWarnEnabled() {

    return LibT::getTraceLevel() >= UaTrace::Warning;
}

bool ClientSdkLogger::isInfoEnabled() {

    return LibT::getTraceLevel() >= UaTrace::Info;
}

bool ClientSdkLogger::isDebugEnabled() {

    return LibT::getTraceLevel() >= UaTrace::InterfaceCall; // incl. CTorDtor
}

bool ClientSdkLogger::isTraceEnabled() {

    return LibT::getTraceLevel() >= UaTrace::ProgramFlow; // incl. Data
}

void ClientSdkLogger::println(const char* level, const char* loggerName, const char* msg,
        const char* msgPrefix, const char* msgSuffix) {
    switch (level[0]) {
        case 'E':
            LibT::lError("%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'W':
            LibT::lWarning("%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'I':
            LibT::lInfo("%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'D':
            LibT::lIfCall("%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
        case 'T':
            LibT::lInOut("%s %s: %s%s%s", level, loggerName, msgPrefix, msg, msgSuffix);
            break;
    }
}