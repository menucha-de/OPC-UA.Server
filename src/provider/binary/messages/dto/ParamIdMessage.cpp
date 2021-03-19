#include "ParamIdMessage.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ParamIdMessagePrivate {
    friend class ParamIdMessage;
private:
    MessageHeader* messageHeader;
    const ParamId* paramId;
    bool hasAttachedValues;
};

ParamIdMessage::ParamIdMessage(MessageHeader::MessageType messageType,
        unsigned long messageId, const ParamId& paramId, bool attachValues) {
    d = new ParamIdMessagePrivate();
    d->messageHeader = new MessageHeader(messageType, messageId);
    d->paramId = &paramId;
    d->hasAttachedValues = attachValues;
}

ParamIdMessage::ParamIdMessage(const ParamIdMessage& orig) {
    d = new ParamIdMessagePrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->paramId = new ParamId(*orig.d->paramId);
    d->hasAttachedValues = true;
}

ParamIdMessage::~ParamIdMessage() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        delete d->paramId;
    }
    delete d;
}

const ParamId& ParamIdMessage::getParamId() const {
    return *d->paramId;
}

void ParamIdMessage::setParamId(const ParamId& paramId) {
    if (d->hasAttachedValues) {
        delete d->paramId;
    }
    d->paramId = &paramId;
}

MessageHeader& ParamIdMessage::getMessageHeader() {
    return *d->messageHeader;
}
