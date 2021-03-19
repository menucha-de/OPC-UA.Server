#include "Call.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class CallPrivate {
    friend class Call;
private:
    MessageHeader* messageHeader;
    const ParamId* methodId;
    const ParamId* paramId;
    const ParamList* paramList;
    bool hasAttachedValues;
};

Call::Call(unsigned long messageId, const ParamId& methodId, const ParamId& paramId,
        const ParamList& paramList, bool attachValues) {
    d = new CallPrivate();
    d->messageHeader = new MessageHeader(MessageHeader::CALL, messageId);
    d->methodId = &methodId;
    d->paramId = &paramId;
    d->paramList = &paramList;
    d->hasAttachedValues = attachValues;
}

Call::Call(const Call& orig) {
    d = new CallPrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->methodId = new ParamId(*orig.d->methodId);
    d->paramId = new ParamId(*orig.d->paramId);
    d->paramList = new ParamList(*orig.d->paramList);
    d->hasAttachedValues = true;
}

Call::~Call() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        delete d->methodId;
        delete d->paramId;
        delete d->paramList;
    }
    delete d;
}

const ParamId& Call::getMethodId() const {
    return *d->methodId;
}

void Call::setMethodId(const ParamId& methodId) {
    if (d->hasAttachedValues) {
        delete d->methodId;
    }
    this->d->methodId = &methodId;
}

const ParamId& Call::getParamId() const {
    return *d->paramId;
}

void Call::setParamId(const ParamId& paramId) {
    if (d->hasAttachedValues) {
        delete d->paramId;
    }
    d->paramId = &paramId;
}

const ParamList& Call::getParamList() const {
    return *d->paramList;
}

void Call::setParamList(const ParamList& paramList) {
    if (d->hasAttachedValues) {
        delete d->paramList;
    }
    d->paramList = &paramList;
}

MessageHeader& Call::getMessageHeader() {
    return *d->messageHeader;
}

Message* Call::copy() const {
    return new Call(*this);
}
