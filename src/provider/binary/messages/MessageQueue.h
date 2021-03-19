#ifndef PROVIDER_BINARY_MESSAGES_MESSAGEQUEUE_H
#define PROVIDER_BINARY_MESSAGES_MESSAGEQUEUE_H

#include "../messages/dto/Message.h"

class MessageQueuePrivate;

// This class is thread safe.

class MessageQueue {
public:
    MessageQueue(int maxSize, bool attachValues = false)/* throws MutexException */;
    // Added messages are deleted.
    virtual ~MessageQueue();

    // time out in seconds
    virtual void put(Message& message, unsigned int timeout) /* throws TimeoutException */;
    // time out in seconds
    virtual Message* get(unsigned long messageId,
            unsigned int timeout)/* throws TimeoutException */;
    virtual int getSize();
private:
    MessageQueuePrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_MESSAGEQUEUE_H */
