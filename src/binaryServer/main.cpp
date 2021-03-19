#include "Server.h"
#include "../utilities/linux.h" // SHUTDOWN_SEQUENCE
#include <common/Exception.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <uathread.h> // UaThread
#include <getopt.h> // getopt_long
#include <string>
#ifdef DEBUG
#include <CppUTest/TestHarness.h>
#endif

using namespace CommonNamespace;

void trim(std::string& str, std::string** returnStr) {
    size_t startIndex = str.find_first_not_of(" ", 0 /* startIndex */);
    size_t endIndex = str.find_last_not_of(" ");
    *returnStr = new std::string(str.substr(startIndex, endIndex - startIndex + 1));
}

int main(int argc, char** argv) {
#ifdef DEBUG
    // avoid memory leak detection while running the server
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
#endif    

    // parse options
    Server::Options options;
    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"server-port", required_argument, NULL, 'p'},
        {"remote-host", required_argument, NULL, 'H'},
        {"remote-port", required_argument, NULL, 'P'},
        {"remote-auth", required_argument, NULL, 'A'},
        {"logging-file-path", required_argument, NULL, 'l'},        
        {0, 0, 0, 0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "hp:H:P:A:l:",
            long_options, NULL /* optionIndex */)) != -1) {
        switch (c) {
            case 'h': // --help
                printf("Usage: %s [options]\n"
                        "Options:\n"
                        "  -h, --help\n"
                        "  -p, --server-port\n"
                        "  -H, --remote-host\n"
                        "  -P, --remote-port\n"
                        "  -A, --remote-auth <key>=<value> with keys 'username','password'\n"
                        "  -l, --logging-file-path\n",
                        argv[0]);
                return 0;
            case 'p': // --server-port                
                options.serverPort = new int;
                *options.serverPort = atoi(optarg);
                if (*options.serverPort == 0) {
                    fprintf(stderr, "%s: invalid server port: %s\n", argv[0], optarg);
                    return 1;
                }
                break;
            case 'H': // --remote-host
                options.remoteHost = new std::string(optarg);
                break;
            case 'P': // --remote-port
                options.remotePort = new int;
                *options.remotePort = atoi(optarg);
                if (*options.remotePort == 0) {
                    fprintf(stderr, "%s: invalid remote port: %s\n", argv[0], optarg);
                    return 1;
                }
                break;
            case 'A': // --remote-auth
            {
                std::string param(optarg);
                size_t index = param.find_first_of("=", 0 /* startIndex */);                
                if (index == std::string::npos) {
                    fprintf(stderr, "%s: invalid remote auth format: %s (character '=' expected)\n",
                            argv[0], optarg);
                    return 1;
                } else {
                    std::string rawKey(param.substr(0, index));                    
                    std::string* key;
                    trim(rawKey, &key);                    
                    if (key->compare("username") == 0) {
                        options.username = new std::string(param.substr(index + 1));                        
                    } else if (key->compare("password") == 0) {
                        options.password = new std::string(param.substr(index + 1));                        
                    } else {
                        fprintf(stderr, "%s: invalid remote auth key: %s ('username' or 'password' expected)\n",
                                argv[0], key->c_str());
                        return 1;
                    }
                    delete key;
                }
                break;
            }
            case 'l': // --logging-file-path                
            {
                // trim path
                std::string loggingFilePath(optarg);                
                size_t startIndex = loggingFilePath.find_first_not_of(" ", 0 /* startIndex */);
                size_t endIndex = loggingFilePath.find_last_not_of(" ");
                options.loggingFilePath = new std::string(
                        loggingFilePath.substr(startIndex, endIndex - startIndex + 1));
                break;
            }
            default:
                /* getopt already printed an error message to stderr */
                return 1;
        }
    }    
    if (options.username == NULL && options.password != NULL
            || options.username != NULL && options.password == NULL) {
        fprintf(stderr, "%s: missing remote auth key ('username' and 'password' expected)\n", 
                argv[0]);
        return 1;
    }

    // initialize logging
    ConsoleLoggerFactory consoleLoggerFactory;
    LoggerFactory loggerFactory(consoleLoggerFactory);
    Logger* log = LoggerFactory::getLogger("main");
    try {
        Server server;
        server.open(options); // ServerException, ServerSocketException, HaSessionException
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
        server.close(); // HaSessionException, HaSubscriptionException        
    } catch (Exception& e) {
        std::string st;
        e.getStackTrace(st);
        log->error("Exception: %s", st.c_str());
        return 1;
    }
    return 0;
}
