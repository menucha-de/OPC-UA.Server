#include "MessageHeader.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class MessageHeaderPrivate {
    friend class MessageHeader;
private:
    MessageHeader::MessageType messageType;
    unsigned long messageBodyLength;
    unsigned long messageId;
};

MessageHeader::MessageHeader(MessageType messageType, unsigned long messageId) {
    d = new MessageHeaderPrivate();
    d->messageType = messageType;
    d->messageBodyLength = 0;
    d->messageId = messageId;
}

MessageHeader::MessageHeader(const MessageHeader& orig) {
    d = new MessageHeaderPrivate();
    d->messageType = orig.d->messageType;
    d->messageBodyLength = orig.d->messageBodyLength;
    d->messageId = orig.d->messageId;
}

MessageHeader::~MessageHeader() {
    delete d;
}

MessageHeader::MessageType MessageHeader::getMessageType() const {
    return d->messageType;
}

unsigned long MessageHeader::getMessageBodyLength() const {
    return d->messageBodyLength;
}

void MessageHeader::setMessageBodyLength(unsigned long messageBodyLength) {
    d->messageBodyLength = messageBodyLength;
}

unsigned long MessageHeader::getMessageId() const {
    return d->messageId;
}
