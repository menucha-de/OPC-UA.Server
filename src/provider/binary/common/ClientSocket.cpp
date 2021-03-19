#include "ClientSocket.h"
#include "ClientSocketException.h"
#include "TimeoutException.h"
#include "../../../utilities/linux.h" // getTimeString
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <netdb.h> // gethostbyname
#include <netinet/in.h> // socket
#include <stddef.h> // NULL
#include <string.h> //memset
namespace UnistdNamespace {
#include <unistd.h> // close
}
#include <sstream> //std::ostringstream
#include <fcntl.h>
#include <errno.h>

using namespace CommonNamespace;

class ClientSocketPrivate {
    friend class ClientSocket;
private:
    Logger* log;
    bool reuseAddress;
    int csockfd;
    int connectTimeout;
    int sendReceiveTimeout;
    Mutex* mutex;
    int pipeFds[2];
    bool isPipeOpened;

    // timeout in seconds
    void connectFd(int sockfd, sockaddr_in* addr, int timeout);
};

ClientSocket::ClientSocket(int connectTimeout, int sendReceiveTimeout, bool reuseAddress) /* throws MutexException */ {
    d = new ClientSocketPrivate();
    d->log = LoggerFactory::getLogger("ClientSocket");
    d->reuseAddress = reuseAddress;
    d->csockfd = -1;
    d->connectTimeout = connectTimeout;
    d->sendReceiveTimeout = sendReceiveTimeout;
    d->mutex = new Mutex();
    d->isPipeOpened = false;
}

ClientSocket::~ClientSocket() {
    close();
    delete d->mutex;
    delete d;
}

void ClientSocket::setConnectTimeout(int timeout) {
    MutexLock lock(*d->mutex);
    d->connectTimeout = timeout;
}

void ClientSocket::open(std::string& host,
        int port) /* throws TimeoutException, ClientSocketException */ {
    d->log->info("Opening connection to %s:%d", host.c_str(), port);
    int csockfd;
    int pipeReadFd;
    int connectTimeout;
    {
        MutexLock lock(*d->mutex);
        if (d->isPipeOpened) {
            // clear pipe and close it
            char ch;
            while (0 < UnistdNamespace::read(d->pipeFds[0], &ch, 1)) {
            }
            UnistdNamespace::close(d->pipeFds[0]);
            UnistdNamespace::close(d->pipeFds[1]);
            d->isPipeOpened = false;
        }
        // self pipe trick:
        // - create an internal pipe
        // - make read + write end nonblocking
        // - add read end of pipe to read fd set for select call
        // - write anything to pipe to abort select call
        if (UnistdNamespace::pipe(d->pipeFds) == -1) {
            throw ExceptionDef(ClientSocketException, std::string("Cannot create internal pipe"));
        }
        fcntl(d->pipeFds[0], F_SETFL, O_NONBLOCK);
        fcntl(d->pipeFds[1], F_SETFL, O_NONBLOCK);
        d->isPipeOpened = true;

        struct hostent* server = gethostbyname(host.c_str());
        if (server == NULL) {
            throw ExceptionDef(ClientSocketException, std::string("Invalid host name ").append(host));
        }
        // create non blocking socket    
        d->csockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (d->csockfd < 0) {
            std::ostringstream msg;
            msg << "Cannot open a client socket for " << host << ":" << port;
            throw ExceptionDef(ClientSocketException, msg.str());
        }
        if (d->reuseAddress) {
            int optval = 1;
            if (setsockopt(d->csockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int)) < 0) {
                close();
                std::ostringstream msg;
                msg << "Cannot set SO_REUSEADDR option to client socket";
                throw ExceptionDef(ClientSocketException, msg.str());
            }
        }
        fcntl(d->csockfd, F_SETFL, O_NONBLOCK);
        // start connecting to server
        struct sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof (servAddr));
        servAddr.sin_family = AF_INET;
        memcpy((char *) &servAddr.sin_addr.s_addr, (char *) server->h_addr,
                server->h_length);
        servAddr.sin_port = htons(port);
        int rc = connect(d->csockfd, (struct sockaddr *) &servAddr, sizeof (servAddr));
        if ((rc == -1) && (errno != EINPROGRESS)) {
            close();
            std::ostringstream msg;
            msg << "Cannot connect to " << host << ":" << port << " (" << strerror(errno) << ")";
            throw ExceptionDef(ClientSocketException, msg.str());
        }
        if (rc == 0) {
            // connection has succeeded immediately
            return;
        }

        csockfd = d->csockfd;
        pipeReadFd = d->pipeFds[0];
        connectTimeout = d->connectTimeout;
    } // MutexLock

    fd_set writeFdSet;
    FD_ZERO(&writeFdSet);
    FD_SET(csockfd, &writeFdSet);
    int highestSockfd = csockfd;

    fd_set readFdSet;
    FD_ZERO(&readFdSet);
    // add read end of pipe to read fd set
    FD_SET(pipeReadFd, &readFdSet);
    // adjust highestSockfd
    if (pipeReadFd > highestSockfd) {
        highestSockfd = pipeReadFd;
    }

    timeval tv;
    tv.tv_sec = connectTimeout;
    tv.tv_usec = 0;
    int count = select(highestSockfd + 1, &readFdSet, &writeFdSet, NULL,
            connectTimeout <= 0 ? NULL : &tv);
    if (FD_ISSET(pipeReadFd, &readFdSet)) {
        throw ExceptionDef(ClientSocketException,
                std::string("Cannot connect to server due to a closed socket"));
    }
    switch (count) {
        case 1:
        {
            int so_error;
            socklen_t len = sizeof (so_error);
            getsockopt(csockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error != 0) {
                close();
                std::ostringstream msg;
                msg << "Cannot connect to " << host << ":" << port
                        << " (" << strerror(so_error) << ")";
                throw ExceptionDef(ClientSocketException, msg.str());
            }
            break;
        }
        case 0:
        {// timeout
            close();
            std::ostringstream msg;
            msg << "Cannot connect to " << host << ":" << port
                    << " (time out after " << connectTimeout << " sec.)";
            throw ExceptionDef(TimeoutException, msg.str());
        }
    }
}

void ClientSocket::close() {
    MutexLock lock(*d->mutex);
    if (d->csockfd >= 0) {
        // write a char to the pipe
        UnistdNamespace::write(d->pipeFds[1], "x", 1);
        d->log->info("Closing client socket");
        UnistdNamespace::close(d->csockfd);
        d->csockfd = -1;
    }
}

void ClientSocket::readData(size_t byteCount,
        unsigned char* returnBuffer) /* throws TimeoutException, ClientSocketException */ {
    size_t readCount = 0;
    while (readCount < byteCount) {
        d->log->info("Waiting for %lu bytes", byteCount - readCount);
        int csfd;
        int pipeReadFd;
        {
            MutexLock lock(*d->mutex);
            csfd = d->csockfd;
            pipeReadFd = d->pipeFds[0];
        }

        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        FD_SET(csfd, &readFdSet);
        int highestSockfd = csfd;

        // add read end of pipe to read fd set
        FD_SET(pipeReadFd, &readFdSet);
        // adjust highestSockfd
        if (pipeReadFd > highestSockfd) {
            highestSockfd = pipeReadFd;
        }

        timeval tv;
        tv.tv_sec = d->sendReceiveTimeout;
        tv.tv_usec = 0;
        int count = select(highestSockfd + 1, &readFdSet /*read*/, NULL /*write*/, NULL,
                d->sendReceiveTimeout <= 0 ? NULL : &tv);
        if (FD_ISSET(pipeReadFd, &readFdSet)) {
            throw ExceptionDef(ClientSocketException,
                    std::string("Cannot read data due to a closed socket"));
        }
        switch (count) {
            case 1:
            {
                int so_error;
                socklen_t len = sizeof (so_error);
                getsockopt(csfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error != 0) {
                    close();
                    std::ostringstream msg;
                    msg << "Cannot read from socket (" << strerror(so_error) << ")";
                    throw ExceptionDef(ClientSocketException, msg.str());
                }
                break;
            }
            case 0:
            {// timeout
                std::ostringstream msg;
                msg << "Cannot read from socket (time out after " << d->sendReceiveTimeout
                        << " sec.)";
                throw ExceptionDef(TimeoutException, msg.str());
            }
        }

        int n = UnistdNamespace::read(csfd, returnBuffer + readCount, byteCount - readCount);
        if (n == 0) {
            close();
            std::ostringstream msg;
            msg << "Connection closed by remote side";
            throw ExceptionDef(ClientSocketException, msg.str());
        } else if (n < 0) {
            int so_error;
            socklen_t len = sizeof (so_error);
            getsockopt(csfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error != 0) {
                close();
                std::ostringstream msg;
                msg << "Cannot read from socket (" << strerror(so_error) << ")";
                throw ExceptionDef(ClientSocketException, msg.str());
            }
        } else {
            d->log->info("Read %d bytes", n);
            readCount += n;
        }
    }
}

void ClientSocket::writeData(size_t byteCount,
        unsigned char* buffer) /* throws TimeoutException, ClientSocketException */ {
    size_t writeCount = 0;
    while (writeCount < byteCount) {
        int csfd;
        int pipeReadFd;
        {
            MutexLock lock(*d->mutex);
            csfd = d->csockfd;
            pipeReadFd = d->pipeFds[0];
        }

        fd_set writeFdSet;
        FD_ZERO(&writeFdSet);
        FD_SET(csfd, &writeFdSet);
        int highestSockfd = csfd;

        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        // add read end of pipe to read fd set
        FD_SET(pipeReadFd, &readFdSet);
        // adjust highestSockfd
        if (pipeReadFd > highestSockfd) {
            highestSockfd = pipeReadFd;
        }

        timeval tv;
        tv.tv_sec = d->sendReceiveTimeout;
        tv.tv_usec = 0;
        int count = select(highestSockfd + 1, &readFdSet, &writeFdSet /*write*/, NULL,
                d->sendReceiveTimeout <= 0 ? NULL : &tv);
        if (FD_ISSET(pipeReadFd, &readFdSet)) {
            throw ExceptionDef(ClientSocketException,
                    std::string("Cannot write data due to a closed socket"));
        }
        switch (count) {
            case 1:
            {
                int so_error;
                socklen_t len = sizeof (so_error);
                getsockopt(csfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error != 0) {
                    close();
                    std::ostringstream msg;
                    msg << "Cannot write to socket (" << strerror(so_error) << ")";
                    throw ExceptionDef(ClientSocketException, msg.str());
                }
                break;
            }
            case 0:
            {// timeout
                std::ostringstream msg;
                msg << "Cannot write to socket (time out after " << d->sendReceiveTimeout << " sec.)";
                throw ExceptionDef(TimeoutException, msg.str());
            }
        }

        int n = UnistdNamespace::write(csfd, buffer + writeCount, byteCount - writeCount);
        if (n == 0) {
            close();
            throw ExceptionDef(ClientSocketException, "Connection closed by remote side");
        } else if (n < 0) {
            int so_error;
            socklen_t len = sizeof (so_error);
            getsockopt(csfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error != 0) {
                close();
                std::ostringstream msg;
                msg << "Cannot write to socket (" << strerror(so_error) << ")";
                throw ExceptionDef(ClientSocketException, msg.str());
            }
        } else {
            d->log->info("Wrote %d bytes", n);
            writeCount += n;
        }
    }
}
