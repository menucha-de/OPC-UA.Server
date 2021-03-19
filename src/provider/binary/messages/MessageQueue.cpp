#include "MessageQueue.h"
#include "../common/TimeoutException.h"
#include <common/Mutex.h>
#include <common/MutexException.h>
#include <common/MutexLock.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <map>
#include <pthread.h> // pthread_cond_t
#include <sstream> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class MessageQueuePrivate {
    friend class MessageQueue;
private:
    Logger* log;
    int maxSize;
    bool hasAttachedValues;
    std::map<unsigned long, Message*> queue;
    Mutex* mutex;
    pthread_cond_t condition;
};

MessageQueue::MessageQueue(int maxSize, bool attachValues) /* throws MutexException */ {
    d = new MessageQueuePrivate();
    d->log = LoggerFactory::getLogger("MessageQueue");
    d->maxSize = maxSize;
    d->hasAttachedValues = attachValues;
    d->mutex = new Mutex(); // MutexException
    if (pthread_cond_init(&d->condition, NULL) != 0) {
        throw ExceptionDef(MutexException, "Cannot initialize mutex condition");
    }
}

MessageQueue::~MessageQueue() {
    if (d->hasAttachedValues) {
        // delete all messages
        for (std::map<unsigned long, Message*>::iterator i = d->queue.begin();
                i != d->queue.end(); i++) {
            d->log->info("Deleting message with identifier %ld", (*i).first);
            delete (*i).second;
        }
    }
    delete d->mutex;
    delete d;
}

void MessageQueue::put(Message& message, unsigned int timeout) /* throws TimeoutException */ {
    timespec to;
    to.tv_sec = time(NULL) + timeout;
    to.tv_nsec = 0;
    MutexLock lock(*d->mutex);
    int err = 0;
    while (err == 0 && (int) d->queue.size() == d->maxSize) {
        if (timeout <= 0) {
            err = pthread_cond_wait(&d->condition, &d->mutex->getMutex());
        } else {
            err = pthread_cond_timedwait(&d->condition, &d->mutex->getMutex(), &to);
        }
    }
    if (err == 0) {
        d->queue[message.getMessageHeader().getMessageId()] = &message;
        pthread_cond_broadcast(&d->condition);
    } else {
        std::ostringstream msg;
        msg << "Cannot add message with id "
                << message.getMessageHeader().getMessageId()
                << " to queue due to time out after " << timeout << "seconds";
        throw ExceptionDef(TimeoutException, msg.str());
    }
}

Message* MessageQueue::get(unsigned long messageId,
        unsigned int timeout) /* throws TimeoutException */ {
    timespec to;
    to.tv_sec = time(NULL) + timeout;
    to.tv_nsec = 0;
    Message* msg = NULL;
    int err = 0;
    MutexLock lock(*d->mutex);
    while (err == 0 && msg == NULL) {
        std::map<unsigned long, Message*>::iterator i = d->queue.find(
                messageId);
        if (i == d->queue.end()) {
            if (timeout <= 0) {
                err = pthread_cond_wait(&d->condition, &d->mutex->getMutex());
            } else {
                err = pthread_cond_timedwait(&d->condition, &d->mutex->getMutex(), &to);
            }
        } else {
            msg = (*i).second;
            d->queue.erase(i);
        }
    }
    if (err == 0) {
        pthread_cond_signal(&d->condition);
    } else {
        std::ostringstream msg;
        msg << "Cannot get message with id " << messageId
                << " from queue due to time out after " << timeout << " seconds";
        throw ExceptionDef(TimeoutException, msg.str());
    }
    return msg;
}

int MessageQueue::getSize() {
    MutexLock lock(*d->mutex);
    return d->queue.size();
}
