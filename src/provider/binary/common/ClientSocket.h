#ifndef PROVIDER_BINARY_COMMON_CLIENTSOCKET_H
#define PROVIDER_BINARY_COMMON_CLIENTSOCKET_H

#include <string>

class ClientSocketPrivate;

// The class is thread safe.

class ClientSocket {
public:
    // connectTimeout: in seconds
    // sendReceiveTimeout: in seconds
    ClientSocket(int connectTimeout, int sendReceiveTimeout, bool reuseAddress = false) /* throws MutexException */;
    virtual ~ClientSocket();

    // timeout: in seconds
    virtual void setConnectTimeout(int timeout);
    virtual void open(std::string& host, int port)/* throws TimeoutException, ClientSocketException */;
    virtual void close();

    // ClientSocketException: the socket is closed
    virtual void readData(size_t byteCount,
            unsigned char* returnBuffer)/* throws TimeoutException, ClientSocketException */;
    // ClientSocketException: the socket is closed
    virtual void writeData(size_t byteCount,
            unsigned char* buffer)/* throws TimeoutException, ClientSocketException */;
private:
    ClientSocketPrivate* d;
};

#endif /* PROVIDER_BINARY_COMMON_CLIENTSOCKET_H */
