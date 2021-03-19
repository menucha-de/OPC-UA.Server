#include "Write.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class WritePrivate {
    friend class Write;
private:
    MessageHeader* messageHeader;
    const ParamId* paramId;
    const Variant* paramValue;
    bool hasAttachedValues;
};

Write::Write(unsigned long messageId, const ParamId& paramId,
        const Variant& paramValue, bool attachValues) {
    d = new WritePrivate();
    d->messageHeader = new MessageHeader(MessageHeader::WRITE, messageId);
    d->paramId = &paramId;
    d->paramValue = &paramValue;
    d->hasAttachedValues = attachValues;
}

Write::Write(const Write& orig) {
    d = new WritePrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->paramId = new ParamId(*orig.d->paramId);
    d->paramValue = orig.d->paramValue->copy();
    d->hasAttachedValues = true;
}

Write::~Write() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        delete d->paramId;
        delete d->paramValue;
    }
    delete d;
}

const ParamId& Write::getParamId() const {
    return *d->paramId;
}

void Write::setParamId(const ParamId& paramId) {
    if (d->hasAttachedValues) {
        delete d->paramId;
    }
    d->paramId = &paramId;
}

const Variant& Write::getParamValue() const {
    return *d->paramValue;
}

void Write::setParamValue(const Variant& paramValue) {
    if (d->hasAttachedValues) {
        delete d->paramValue;
    }
    d->paramValue = &paramValue;
}

MessageHeader& Write::getMessageHeader() {
    return *d->messageHeader;
}

Message* Write::copy() const {
    return new Write(*this);
}
