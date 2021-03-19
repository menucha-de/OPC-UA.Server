#include "ReadResponse.h"
#include "Scalar.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ReadResponsePrivate {
    friend class ReadResponse;
private:
    MessageHeader* messageHeader;
    Status::Value status;
    const ParamId* paramId;
    const Variant* paramValue;
    bool hasAttachedValues;
};

ReadResponse::ReadResponse(unsigned long messageId, Status::Value status,
        bool attachValues) {
    d = new ReadResponsePrivate();
    d->messageHeader = new MessageHeader(MessageHeader::READ_RESPONSE,
            messageId);
    d->status = status;
    d->paramId = NULL;
    d->paramValue = NULL;
    d->hasAttachedValues = attachValues;
}

ReadResponse::ReadResponse(const ReadResponse& orig) {
    d = new ReadResponsePrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->status = orig.d->status;
    d->paramId = orig.d->paramId == NULL ? NULL : new ParamId(*orig.d->paramId);
    d->paramValue = orig.d->paramValue == NULL ? NULL : orig.d->paramValue->copy();
    d->hasAttachedValues = true;
}

ReadResponse::~ReadResponse() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        if (d->paramId != NULL) {
            delete d->paramId;
        }
        if (d->paramValue != NULL) {
            delete d->paramValue;
        }
    }
    delete d;
}

Status::Value ReadResponse::getStatus() const {
    return d->status;
}

void ReadResponse::setStatus(Status::Value status) {
    d->status = status;
}

const ParamId* ReadResponse::getParamId() const {
    return d->paramId;
}

void ReadResponse::setParamId(const ParamId* paramId) {
    if (d->hasAttachedValues && d->paramId != NULL) {
        delete d->paramId;
    }
    d->paramId = paramId;
}

const Variant* ReadResponse::getParamValue() const {
    return d->paramValue;
}

void ReadResponse::setParamValue(const Variant* paramValue) {
    if (d->hasAttachedValues && d->paramValue != NULL) {
        delete d->paramValue;
    }
    d->paramValue = paramValue;
}

MessageHeader& ReadResponse::getMessageHeader() {
    return *d->messageHeader;
}

Message* ReadResponse::copy() const {
    return new ReadResponse(*this);
}
