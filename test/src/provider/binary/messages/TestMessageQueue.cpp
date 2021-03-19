#include "CppUTest/TestHarness.h"
#include "../../../../../src/provider/binary/common/TimeoutException.h"
#include "../../../../../src/provider/binary/messages/MessageQueue.h"
#include "../../../../../src/provider/binary/messages/dto/ParamId.h"
#include "../../../../../src/provider/binary/messages/dto/Read.h"
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <string>
#include <pthread.h> // pthread_t
#include <time.h> //nanosleep

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(ProviderBinaryMessages_MessageQueue) {
        ConsoleLoggerFactory clf;
        LoggerFactory* lf;

        void setup() {
            lf = new LoggerFactory(clf);
        }

        void teardown() {
            delete lf;
        }

        static void* putGetThreadRun(void* object) {
            MessageQueue& queue = *(MessageQueue*) object;
            unsigned long messageId = 3;
            // the "get" call is blocked until a message is added to the queue
            Message* msg = queue.get(messageId, 3 /*timeout*/);
            CHECK_EQUAL(messageId, msg->getMessageHeader().getMessageId());
        }
    };

    TEST(ProviderBinaryMessages_MessageQueue, PutGet) {
        // create a queue
        MessageQueue queue(3 /* maxSize */, false /* attachValues*/);
        CHECK_EQUAL(0, queue.getSize());
        // start thread for reading a message from the queue
        pthread_t thread;
        pthread_create(&thread, NULL, &putGetThreadRun, &queue);
        // wait a second
        timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL /* remaining */);
        // add a message
        ParamId paramId(4);
        unsigned long messageId = 3;
        Read readRequest(messageId, paramId);
        queue.put(readRequest, 1 /*timeout*/);
        // wait for end of thread and check if the message has been removed
        pthread_join(thread, NULL);
        CHECK_EQUAL(0, queue.getSize());

        // create a queue which deletes messages in destructor
        MessageQueue queue2(3 /* maxSize */, true /* attachValues*/);
        // add a message
        ParamId paramId2(4);
        unsigned long messageId2 = 3;
        Read* readRequest2 = new Read(messageId2, paramId2);
        queue2.put(*readRequest2, 1 /*timeout*/);
        // the destructor deletes the message (check via memory leak detector)
    }

    TEST(ProviderBinaryMessages_MessageQueue, PutErrors) {
        // create a queue for max. 1 message
        MessageQueue queue(1 /* maxSize */);
        // add a message
        ParamId paramId(4);
        unsigned long messageId = 3;
        Read readRequest(messageId, paramId);
        queue.put(readRequest, 1 /*timeout*/);
        // try to add a further message
        time_t start = time(NULL);
        try {
            queue.put(readRequest, 2 /*timeout*/);
            FAIL("");
        } catch (TimeoutException& e) {
            time_t end = time(NULL);
            STRCMP_CONTAINS("Cannot add message", e.getMessage().c_str());
            // check the time out
            time_t diff = end - start;
            CHECK(diff >= 1 && diff <= 3);
        }
    }

    TEST(ProviderBinaryMessages_MessageQueue, GetErrors) {
        // create an empty queue
        MessageQueue queue(1 /* maxSize */);
        CHECK_EQUAL(0, queue.getSize());
        // try to get a message from the empty queue
        time_t start = time(NULL);
        try {
            queue.get(1 /* messageId*/, 2 /*timeout*/);
            FAIL("");
        } catch (TimeoutException& e) {
            time_t end = time(NULL);
            STRCMP_CONTAINS("Cannot get message", e.getMessage().c_str());
            // check the time out
            time_t diff = end - start;
            CHECK(diff >= 1 && diff <= 3);
        }
    }
}
