#include "StatusMessage.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class StatusMessagePrivate {
    friend class StatusMessage;
private:
    MessageHeader* messageHeader;
    Status::Value status;
};

StatusMessage::StatusMessage(MessageHeader::MessageType messageType,
        unsigned long messageId, Status::Value status) {
    d = new StatusMessagePrivate();
    d->messageHeader = new MessageHeader(messageType, messageId);
    d->status = status;
}

StatusMessage::StatusMessage(const StatusMessage& orig) {
    d = new StatusMessagePrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->status = orig.d->status;
}

StatusMessage::~StatusMessage() {
    delete d->messageHeader;
    delete d;
}

Status::Value StatusMessage::getStatus() const {
    return d->status;
}

void StatusMessage::setStatus(Status::Value status) {
    d->status = status;
}

MessageHeader& StatusMessage::getMessageHeader() {
    return *d->messageHeader;
}
