#include "Notification.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class NotificationPrivate {
    friend class Notification;
private:
    MessageHeader* messageHeader;
    const ParamMap* paramMap;
    bool hasAttachedValues;
};

Notification::Notification(unsigned long messageId, const ParamMap& paramMap,
        bool attachValues) {
    d = new NotificationPrivate();
    d->messageHeader = new MessageHeader(MessageHeader::NOTIFICATION,
            messageId);
    d->paramMap = &paramMap;
    d->hasAttachedValues = attachValues;
}

Notification::Notification(const Notification& orig) {
    d = new NotificationPrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->paramMap = new ParamMap(*orig.d->paramMap);
    d->hasAttachedValues = true;
}

Notification::~Notification() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        delete d->paramMap;
    }
    delete d;
}

const ParamMap& Notification::getParamMap() const {
    return *d->paramMap;
}

void Notification::setParamMap(const ParamMap& paramMap) {
    if (d->hasAttachedValues) {
        delete d->paramMap;
    }
    d->paramMap = &paramMap;
}

MessageHeader& Notification::getMessageHeader() {
    return *d->messageHeader;
}

Message* Notification::copy() const {
    return new Notification(*this);
}
