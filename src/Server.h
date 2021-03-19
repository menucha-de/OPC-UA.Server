#ifndef SERVER_H
#define SERVER_H

#include <ioDataProvider/IODataProviderFactory.h>
#include <sasModelProvider/SASModelProviderFactory.h>
#include <map>
#include <string>

class ServerPrivate;

class Server {
public:
    // shutdownDelay: allow clients to disconnect after they received the shutdown signal 
    //                (in seconds)
    Server(OpcUa_UInt shutdownDelay);
    ~Server();

    void open() /*throws ServerException, IODataProviderException, SASModelProviderException*/;
    void close();
private:
    ServerPrivate* d;
};

#endif /* SERVER_H */

