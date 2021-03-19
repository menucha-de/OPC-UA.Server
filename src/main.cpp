#include "Server.h"
#include "utilities/linux.h" // SHUTDOWN_SEQUENCE
#include <common/Exception.h>
#include <common/logging/ConsoleLogger.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <uathread.h> // UaThread
#ifdef DEBUG
#include <CppUTest/TestHarness.h>
#endif

using namespace CommonNamespace;

class ServerConsoleLogger : public ConsoleLogger {
public:

    ServerConsoleLogger(const char* name) : ConsoleLogger(name) {
    }

    // interface ConsoleLogger

    virtual void println(const char* timeStamp, const char* level, const char* loggerName,
            const char* msg) {
        // print to stderr instead of stdout
        fprintf(stderr, "%s %s %s: %s\n", timeStamp, level, loggerName, msg);
    }
};

class ServerConsoleLoggerFactory : public ConsoleLoggerFactory {

    virtual CommonNamespace::Logger* createLogger(const char* name) {
        return new ServerConsoleLogger(name);
    }
};

int main(int argc, char** argv) {
#ifdef DEBUG
    // avoid memory leak detection while running the server
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
#endif
    // initialize logging
    ServerConsoleLoggerFactory consoleLoggerFactory;
    LoggerFactory loggerFactory(consoleLoggerFactory);
    Logger* log = LoggerFactory::getLogger("main");
    try {
        Server server(0 /*shutdownDelay*/);
        server.open(); // ServerException, IODataProviderException, SASModelProviderException
        printf("***************************************************\n");
        printf(" Press %s to shut down server\n", SHUTDOWN_SEQUENCE);
        printf("***************************************************\n");
        // Wait for user command to terminate the server thread.
        while (getShutDownFlag() == 0) {
            UaThread::msleep(1000);
        }
        printf("***************************************************\n");
        printf(" Shutting down server\n");
        printf("***************************************************\n");
        server.close();
    } catch (Exception& e) {
        std::string st;
        e.getStackTrace(st);
        log->error("Exception: %s", st.c_str());
        return 1;
    }
    return 0;
}
