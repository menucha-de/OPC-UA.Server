#include <common/logging/ConsoleLogger.h>
#include "../../utilities/linux.h" //getTimeStamp
#include <stdarg.h> // va_list
#include <stdio.h> // vasprintf
#include <stdlib.h> // free
#include <string.h> // strdup

using namespace CommonNamespace;

class ConsoleLoggerPrivate {
    friend class ConsoleLogger;
private:
    static const char LINE_DELIMITER[];

    std::string name;
};

const char ConsoleLoggerPrivate::LINE_DELIMITER[] = "\n";

ConsoleLogger::ConsoleLogger(const char* name) {
    d = new ConsoleLoggerPrivate();
    d->name = std::string(name);
}

ConsoleLogger::~ConsoleLogger() {
    delete d;
}

void ConsoleLogger::error(const char* format, ...) {
    if (isErrorEnabled()) {
        char* buffer;
        va_list args;
        va_start(args, format);
        vasprintf(&buffer, format, args);
        va_end(args);

        char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
        while (bufferLine != NULL) {
            char* ts = getTimeString();
            println(stderr, ts, "ERROR", d->name.c_str(), bufferLine);            
            delete[] ts;
            bufferLine = strtok(NULL, d->LINE_DELIMITER);
        }

        free(buffer);
    }
}

void ConsoleLogger::warn(const char* format, ...) {
    if (isWarnEnabled()) {
        char* buffer;
        va_list args;
        va_start(args, format);
        vasprintf(&buffer, format, args);
        va_end(args);

        char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
        while (bufferLine != NULL) {
            char* ts = getTimeString();
            println(stderr, ts, "WARN", d->name.c_str(), bufferLine);
            delete[] ts;
            bufferLine = strtok(NULL, d->LINE_DELIMITER);
        }

        free(buffer);
    }
}

void ConsoleLogger::info(const char* format, ...) {
    if (isInfoEnabled()) {
        char* buffer;
        va_list args;
        va_start(args, format);
        vasprintf(&buffer, format, args);
        va_end(args);

        char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
        while (bufferLine != NULL) {
            char* ts = getTimeString();
            println(stdout, ts, "INFO", d->name.c_str(), bufferLine);
            delete[] ts;
            bufferLine = strtok(NULL, d->LINE_DELIMITER);
        }

        free(buffer);
    }
}

void ConsoleLogger::debug(const char* format, ...) {
    if (isDebugEnabled()) {
        char* buffer;
        va_list args;
        va_start(args, format);
        vasprintf(&buffer, format, args);
        va_end(args);

        char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
        while (bufferLine != NULL) {
            char* ts = getTimeString();
            println(stdout, ts, "DEBUG", d->name.c_str(), bufferLine);
            delete[] ts;
            bufferLine = strtok(NULL, d->LINE_DELIMITER);
        }

        free(buffer);
    }
}

void ConsoleLogger::trace(const char* format, ...) {
    if (isTraceEnabled()) {
        char* buffer;
        va_list args;
        va_start(args, format);
        vasprintf(&buffer, format, args);
        va_end(args);

        char *bufferLine = strtok(buffer, d->LINE_DELIMITER);
        while (bufferLine != NULL) {
            char* ts = getTimeString();
            println(stdout, ts, "TRACE", d->name.c_str(), bufferLine);
            delete[] ts;
            bufferLine = strtok(NULL, d->LINE_DELIMITER);
        }

        free(buffer);
    }
}

bool ConsoleLogger::isErrorEnabled() {
    return true;
}

bool ConsoleLogger::isWarnEnabled() {
    return true;
}

bool ConsoleLogger::isInfoEnabled() {
    return true;
}

bool ConsoleLogger::isDebugEnabled() {
    return true;
}

bool ConsoleLogger::isTraceEnabled() {
    return true;
}

void ConsoleLogger::println(FILE *outputStream, const char* timeStamp, const char* level, const char* loggerName,
        const char* msg) {
    fprintf(outputStream, "%s %s %s: %s\n", timeStamp, level, loggerName, msg);
}