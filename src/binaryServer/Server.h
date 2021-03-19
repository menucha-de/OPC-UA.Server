#ifndef BINARYSERVER_SERVER_H
#define BINARYSERVER_SERVER_H

#include <string>

class ServerPrivate;

class Server {
public:

    class Options {
    public:
        int* serverPort = NULL;
        std::string* remoteHost = NULL;
        int* remotePort = NULL;
        std::string* username = NULL;
        std::string* password = NULL;
        std::string* loggingFilePath = NULL;        
    };

    Server() /* throws MutexException */;
    virtual ~Server() /* throws HaSessionException, HaSubscriptionException */;

    virtual void open(Options& conf) /* throws ServerException, ServerSocketException, HaSessionException */;
    virtual void close() /* throws HaSessionException, HaSubscriptionException */;

private:
    Server(const Server& orig);
    Server& operator=(const Server&);

    ServerPrivate* d;
};

#endif /* BINARYSERVER_SERVER_H */

