#ifndef PROVIDER_BINARY_COMMON_SERVERSOCKET_H
#define PROVIDER_BINARY_COMMON_SERVERSOCKET_H

#include <stddef.h> // size_t

class ServerSocketPrivate;

// The class is thread safe.

class ServerSocket {
public:
    ServerSocket(int acceptTimeout, int sendReceiveTimeout, bool reuseAddress = false) /* throws MutexException */;
    virtual ~ServerSocket();

    virtual void open(int port)/* throws ServerSocketException */;
    virtual void close();
    virtual void closeClient();

    // acceptConnection: true: the method is blocked until a client connects to the server,
    //                   false: an exception is thrown is no client is connected
    virtual void readData(size_t byteCount,
            unsigned char* returnBuffer, bool acceptConnection = true)/* throws TimeoutException, ServerSocketException */;
    // acceptConnection: true: the method is blocked until a client connects to the server,
    //                   false: an exception is thrown is no client is connected
    virtual void writeData(size_t byteCount,
            unsigned char* buffer, bool acceptConnection = true)/* throws TimeoutException, ServerSocketException */;
private:
    ServerSocketPrivate* d;
};

#endif /* PROVIDER_BINARY_COMMON_SERVERSOCKET_H */
