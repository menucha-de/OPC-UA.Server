#include "ServerSocket.h"
#include "ServerSocketException.h"
#include "TimeoutException.h"
#include "../../../utilities/linux.h" // getTimeString
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <netinet/in.h> // socket
#include <stddef.h> // NULL
#include <string.h> //memset
namespace UnistdNamespace {
#include <unistd.h> // close
}
#include <sstream> //std::ostringstream
#include <time.h> //nanosleep
#include <fcntl.h>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class ServerSocketPrivate {
    friend class ServerSocket;
private:
    Logger* log;
    bool reuseAddress;
    int sockfd;
    int csockfd;
    int acceptTimeout;
    int sendReceiveTimeout;
    Mutex* mutex;
    int pipeFds[2];
    bool isPipeOpened;

    virtual int acceptConnection(int sockfd, int pipeReadFd);
};

ServerSocket::ServerSocket(int acceptTimeout, int sendReceiveTimeout, bool reuseAddress) /* throws MutexException */ {
    d = new ServerSocketPrivate();
    d->log = LoggerFactory::getLogger("ServerSocket");
    d->reuseAddress = reuseAddress;
    d->sockfd = -1;
    d->csockfd = -1;
    d->acceptTimeout = acceptTimeout;
    d->sendReceiveTimeout = sendReceiveTimeout;
    d->mutex = new Mutex(); // MutexException
    d->isPipeOpened = false;
}

ServerSocket::~ServerSocket() {
    close();
    delete d->mutex;
    delete d;
}

#include <stdio.h>

void ServerSocket::open(int port) /* throws ServerSocketException */ {
    d->log->info("Opening server socket on port %d", port);
    MutexLock lock(*d->mutex);
    if (d->isPipeOpened) {
        // clear pipe and close it
        char ch;
        while (0 < UnistdNamespace::read(d->pipeFds[0], &ch, 1)) {
        }
        UnistdNamespace::close(d->pipeFds[1]);
        UnistdNamespace::close(d->pipeFds[0]);
        d->isPipeOpened = false;
    }
    // self pipe trick:
    // - create an internal pipe
    // - make read + write end nonblocking
    // - add read end of pipe to read fd set for select call
    // - write anything to pipe to abort select call    
    if (UnistdNamespace::pipe(d->pipeFds) == -1) {
        throw ExceptionDef(ServerSocketException, std::string("Cannot create internal pipe"));
    }
    fcntl(d->pipeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(d->pipeFds[1], F_SETFL, O_NONBLOCK);
    d->isPipeOpened = true;

    // create non blocking socket
    d->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (d->sockfd < 0) {
        std::ostringstream msg;
        msg << "Cannot open a server socket for port " << port;
        throw ExceptionDef(ServerSocketException, msg.str());
    }
    if (d->reuseAddress) {
        int optval = 1;
        if (setsockopt(d->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int)) < 0) {
            close();
            std::ostringstream msg;
            msg << "ServerSocket.open: Cannot set SO_REUSEADDR option to server socket";
            throw ExceptionDef(ServerSocketException, msg.str());
        }
    }
    fcntl(d->sockfd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(d->sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        close();
        std::ostringstream msg;
        msg << "Cannot bind server socket to port " << port;
        throw ExceptionDef(ServerSocketException, msg.str());
    }
    if (listen(d->sockfd, 1 /* maxConnectionCount */) < 0) {
        close();
        std::ostringstream msg;
        msg << "Cannot listen for client connections on port " << port;
        throw ExceptionDef(ServerSocketException, msg.str());
    }
}

void ServerSocket::close() {
    MutexLock lock(*d->mutex);
    if (d->sockfd >= 0) {
        closeClient();
        // write a char to the pipe
        UnistdNamespace::write(d->pipeFds[1], "x", 1);
        d->log->info("Closing server socket");
        UnistdNamespace::close(d->sockfd);
        d->sockfd = -1;
    }
}

void ServerSocket::closeClient() {
    MutexLock lock(*d->mutex);
    if (d->csockfd >= 0) {
        d->log->info("Closing client socket");
        UnistdNamespace::close(d->csockfd);
        d->csockfd = -1;
    }
}

void ServerSocket::readData(size_t byteCount, unsigned char* returnBuffer,
        bool accectConnection) /* throws TimeoutException, ServerSocketException */ {
    size_t readCount = 0;
    while (readCount < byteCount) {
        d->log->info("Waiting for %lu bytes", byteCount - readCount);
        int sfd;
        int csfd;
        int pipeReadFd;
        {
            MutexLock lock(*d->mutex);
            sfd = d->sockfd;
            csfd = d->csockfd;
            pipeReadFd = d->pipeFds[0];
        }
        if (csfd < 0) {
            if (accectConnection) {
                csfd = d->acceptConnection(sfd, pipeReadFd);
            } else {
                std::ostringstream msg;
                msg << "Cannot read from socket (missing client connection)";
                throw ExceptionDef(ServerSocketException, msg.str());
            }
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
            throw ExceptionDef(ServerSocketException,
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
                    throw ExceptionDef(ServerSocketException, msg.str());
                }
                break;
            }
            case 0:
            {// timeout
                std::ostringstream msg;
                msg << "Cannot read from socket (time out after "
                        << d->sendReceiveTimeout << " sec.)";
                throw ExceptionDef(TimeoutException, msg.str());
            }
        }

        int n = UnistdNamespace::read(csfd, returnBuffer + readCount, byteCount - readCount);
        if (n == 0) {
            closeClient();
            std::ostringstream msg;
            msg << "Connection closed by remote side";
            throw ExceptionDef(ServerSocketException, msg.str());
        } else if (n < 0) {
            int so_error;
            socklen_t len = sizeof (so_error);
            getsockopt(csfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error != 0) {
                closeClient();
                std::ostringstream msg;
                msg << "Cannot read from socket (" << strerror(so_error) << ")";
                throw ExceptionDef(ServerSocketException, msg.str());
            }
        } else {
            d->log->info("Read %d bytes", n);
            readCount += n;
        }
    }
}

void ServerSocket::writeData(size_t byteCount, unsigned char* buffer,
        bool acceptConnection) /* throws TimeoutException, ServerSocketException */ {
    size_t writeCount = 0;
    while (writeCount < byteCount) {
        int sfd;
        int csfd;
        int pipeReadFd;
        {
            MutexLock lock(*d->mutex);
            sfd = d->sockfd;
            csfd = d->csockfd;
            pipeReadFd = d->pipeFds[0];
        }
        if (csfd < 0) {
            if (acceptConnection) {
                csfd = d->acceptConnection(sfd, pipeReadFd);
            } else {
                std::ostringstream msg;
                msg << "Cannot write to socket (missing client connection)";
                throw ExceptionDef(ServerSocketException, msg.str());
            }
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
        int count = select(highestSockfd + 1, &readFdSet, &writeFdSet, NULL,
                d->sendReceiveTimeout <= 0 ? NULL : &tv);
        if (FD_ISSET(pipeReadFd, &readFdSet)) {
            throw ExceptionDef(ServerSocketException,
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
                    throw ExceptionDef(ServerSocketException, msg.str());
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
            closeClient();
            std::ostringstream msg;
            msg << "Connection closed by remote side";
            throw ExceptionDef(ServerSocketException, msg.str());
        } else if (n < 0) {
            int so_error;
            socklen_t len = sizeof (so_error);
            getsockopt(csfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error != 0) {
                closeClient();
                std::ostringstream msg;
                msg << "Cannot write to socket (" << strerror(so_error) << ")";
                throw ExceptionDef(ServerSocketException, msg.str());
            }
        } else {
            d->log->info("Wrote %d bytes", n);
            writeCount += n;
        }
    }
}

int ServerSocketPrivate::acceptConnection(int sockfd, int pipeReadFd) {
    fd_set readFdSet;
    FD_ZERO(&readFdSet);
    FD_SET(sockfd, &readFdSet);
    int highestSockfd = sockfd;

    // add read end of pipe to read fd set
    FD_SET(pipeReadFd, &readFdSet);
    // adjust highestSockfd
    if (pipeReadFd > highestSockfd) {
        highestSockfd = pipeReadFd;
    }

    timeval tv;
    tv.tv_sec = acceptTimeout;
    tv.tv_usec = 0;
    int count = select(highestSockfd + 1, &readFdSet, NULL /*write*/, NULL,
            acceptTimeout <= 0 ? NULL : &tv);
    if (FD_ISSET(pipeReadFd, &readFdSet)) {
        throw ExceptionDef(ServerSocketException,
                std::string("Cannot accept a connection due to a closed socket"));
    }
    switch (count) {
        case 1:
        {
            int so_error;
            socklen_t len = sizeof (so_error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error != 0) {
                std::ostringstream msg;
                msg << "Cannot accept connection (" << strerror(so_error) << ")";
                throw ExceptionDef(ServerSocketException, msg.str());
            }
            break;
        }
        case 0:
        {// timeout
            std::ostringstream msg;
            msg << "No connection accepted (time out after " << acceptTimeout << " sec.)";
            throw ExceptionDef(TimeoutException, msg.str());
        }
    }

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof (cli_addr);
    int csfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (csfd < 0) {
        std::ostringstream msg;
        msg << "Cannot accept connection";
        throw ExceptionDef(ServerSocketException, msg.str());
    }
    fcntl(csfd, F_SETFL, O_NONBLOCK);
    MutexLock lock(*mutex);
    csockfd = csfd;
    return csfd;
}
