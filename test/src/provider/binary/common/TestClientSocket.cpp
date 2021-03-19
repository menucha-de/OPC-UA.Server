#include "CppUTest/TestHarness.h"
#include "../../../Env.h"
#include "../../../../../src/provider/binary/common/ClientSocket.h"
#include "../../../../../src/provider/binary/common/ClientSocketException.h"
#include "../../../../../src/provider/binary/common/ServerSocket.h"
#include "../../../../../src/provider/binary/common/ServerSocketException.h"
#include "../../../../../src/provider/binary/common/TimeoutException.h"
#include <common/Exception.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <string>
#include <pthread.h> // pthread_t
#include <time.h> //nanosleep

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(ProviderBinaryCommon_ClientSocket) {
        ConsoleLoggerFactory clf;
        LoggerFactory* lf;

        void setup() {
            lf = new LoggerFactory(clf);
        }

        void teardown() {
            delete lf;
        }

        static void* readErrorsThreadRun(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            // wait a second
            timespec delay;
            delay.tv_sec = 1;
            delay.tv_nsec = 0;
            nanosleep(&delay, NULL /* remaining */);
            // send some data and close the socket
            unsigned char serverWriteBuffer[] = {(unsigned char) 0x1};
            serverSocket.writeData(1, serverWriteBuffer);
            serverSocket.close();
        }

        static void* cancelReadServerThreadRun1(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            unsigned char serverReadBuffer[1];
            try {
                serverSocket.readData(1, serverReadBuffer);
                FAIL("");
            } catch (ServerSocketException& e) {
                STRCMP_CONTAINS("Connection closed by remote side", e.getMessage().c_str());
            } catch (CommonNamespace::Exception& e) {
                FAIL("");
            }
        }

        static void* cancelReadClientThreadRun1(void* object) {
            ClientSocket& clientSocket = *(ClientSocket*) object;
            unsigned char clientReadBuffer[1];
            try {
                clientSocket.readData(1, clientReadBuffer);
                FAIL("");
            } catch (ClientSocketException& e) {
                STRCMP_CONTAINS("Cannot read data due to a closed socket", e.getMessage().c_str());
            } catch (CommonNamespace::Exception& e) {
                FAIL("");
            }
        }

        static void* cancelReadServerThreadRun2(void* object) {
            ServerSocket& serverSocket = *(ServerSocket*) object;
            unsigned char serverReadBuffer[1];
            try {
                serverSocket.readData(1, serverReadBuffer);
                FAIL("");
            } catch (ServerSocketException& e) {
                STRCMP_CONTAINS("Cannot read data due to a closed socket", e.getMessage().c_str());
            } catch (CommonNamespace::Exception& e) {
                FAIL("");
            }
        }

        static void* cancelReadClientThreadRun2(void* object) {
            ClientSocket& clientSocket = *(ClientSocket*) object;
            unsigned char clientReadBuffer[1];
            try {
                clientSocket.readData(1, clientReadBuffer);
                FAIL("");
            } catch (ClientSocketException& e) {
                STRCMP_CONTAINS("Connection closed by remote side", e.getMessage().c_str());
            } catch (CommonNamespace::Exception& e) {
                FAIL("");
            }
        }
    };

    TEST(ProviderBinaryCommon_ClientSocket, OpenCloseReadWrite) {
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1);

        ClientSocket clientSocket(3 /*connectTimeout*/, 3 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        std::string host(Env::HOST);
        clientSocket.open(host, Env::PORT1);

        unsigned char serverWriteBuffer[] = {(unsigned char) 0x1, (unsigned char) 0x2, (unsigned char) 0x3};
        serverSocket.writeData(3, serverWriteBuffer);

        unsigned char clientReadBuffer[3];
        clientSocket.readData(3, clientReadBuffer);
        CHECK_EQUAL(serverWriteBuffer[0], clientReadBuffer[0]);
        CHECK_EQUAL(serverWriteBuffer[1], clientReadBuffer[1]);
        CHECK_EQUAL(serverWriteBuffer[2], clientReadBuffer[2]);

        unsigned char clientWriteBuffer[] = {(unsigned char) 0x4, (unsigned char) 0x5};
        clientSocket.writeData(2, clientWriteBuffer);

        unsigned char serverReadBuffer[3];
        serverSocket.readData(2, serverReadBuffer);
        CHECK_EQUAL(clientWriteBuffer[0], serverReadBuffer[0]);
        CHECK_EQUAL(clientWriteBuffer[1], serverReadBuffer[1]);

        clientSocket.close();
    }

    TEST(ProviderBinaryCommon_ClientSocket, OpenErrors) {
        std::string host("a");

        // try to create a connection to an invalid host
        ClientSocket clientSocket(1 /*connectTimeout*/, 1 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        try {
            clientSocket.open(host, Env::PORT1);
            FAIL("");
        } catch (ClientSocketException& e) {
            STRCMP_CONTAINS("Invalid host name", e.getMessage().c_str());
        }

        // try to create a connection to a non-existing server
        host = std::string(Env::HOST);
        try {
            clientSocket.open(host, Env::PORT1);
            FAIL("");
        } catch (ClientSocketException& e) {
            STRCMP_CONTAINS("Cannot connect to", e.getMessage().c_str());
        }
    }

    TEST(ProviderBinaryCommon_ClientSocket, ReadErrors) {
        // try to read from a non-existing connection
        ClientSocket clientSocket1(1 /*connectTimeout*/, 1 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        unsigned char clientReadBuffer[3];
        try {
            clientSocket1.readData(3, clientReadBuffer);
            FAIL("");
        } catch (TimeoutException& e) {
            STRCMP_CONTAINS("Cannot read from socket", e.getMessage().c_str());
        }

        // open a server
        ServerSocket serverSocket(10 /*acceptTimeout*/, 10 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1);
        // start thread for sending a byte and closing the server
        pthread_t thread;
        pthread_create(&thread, NULL, &readErrorsThreadRun, &serverSocket);
        // open a connection to the server
        ClientSocket clientSocket2(3 /*connectTimeout*/, 2 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        std::string host(Env::HOST);
        clientSocket2.open(host, Env::PORT1);
        try {
            // wait for 3 bytes from server
            clientSocket2.readData(3, clientReadBuffer);
            FAIL("");
        } catch (ClientSocketException& e) {
            STRCMP_CONTAINS("Connection closed by remote side", e.getMessage().c_str());
        }
        pthread_join(thread, NULL /*return*/);
    }

    TEST(ProviderBinaryCommon_ClientSocket, WriteErrors) {
        // try to write to a non-existing connection
        ClientSocket clientSocket(1 /*connectTimeout*/, 1 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        unsigned char clientWriteBuffer[] = {
            (unsigned char) 0x1, (unsigned char) 0x2, (unsigned char) 0x3
        };
        try {
            clientSocket.writeData(1, clientWriteBuffer);
            FAIL("");
        } catch (TimeoutException& e) {
            STRCMP_CONTAINS("Cannot write to socket", e.getMessage().c_str());
        }
    }

    TEST(ProviderBinaryCommon_ClientSocket, CancelRead) {
        // open a server
        ServerSocket serverSocket(10 /*acceptTimeout*/, 3 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        serverSocket.open(Env::PORT1);

        // start thread for reading data on server side
        pthread_t serverThread;
        pthread_create(&serverThread, NULL, &cancelReadServerThreadRun1, &serverSocket);
        // wait a second
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /*remaining*/);
        // open a connection to the server
        ClientSocket clientSocket(3 /*connectTimeout*/, 2 /*sendReceiveTimeout*/,
                true /*reuseAddress*/);
        std::string host(Env::HOST);
        clientSocket.open(host, Env::PORT1);
        // start thread for reading data on client side
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, &cancelReadClientThreadRun1, &clientSocket);
        // wait a second
        nanosleep(&delay, NULL /*remaining*/);
        // close the client socket
        clientSocket.close();
        // the client thread is stopped due to closed client socket
        pthread_join(clientThread, NULL /*return*/);
        // the server thread is stopped after ServerSocketException "Connection closed by remote side"
        pthread_join(serverThread, NULL /*return*/);

        // start thread for reading data on server side
        pthread_create(&serverThread, NULL, &cancelReadServerThreadRun2, &serverSocket);
        // wait a second
        delay.tv_sec = 1;
        nanosleep(&delay, NULL /*remaining*/);
        // open a connection to the server
        clientSocket.open(host, Env::PORT1);
        // start thread for reading data on client side
        pthread_create(&clientThread, NULL, &cancelReadClientThreadRun2, &clientSocket);
        // wait a second
        nanosleep(&delay, NULL /*remaining*/);
        // close the server socket
        serverSocket.close();
        // the client thread is stopped because the connection has been closed by the server
        pthread_join(clientThread, NULL /*return*/);
        // the server thread is stopped due to closed server socket
        pthread_join(serverThread, NULL /*return*/);
    }
}
