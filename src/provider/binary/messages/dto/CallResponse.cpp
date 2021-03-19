#include "CallResponse.h"
#include "MessageHeader.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class CallResponsePrivate {
    friend class CallResponse;
private:
    MessageHeader* messageHeader;
    Status::Value status;
    const ParamId* methodId;
    const ParamId* paramId;
    const ParamList* paramList;
    bool hasAttachedValues;
};

CallResponse::CallResponse(unsigned long messageId, Status::Value status, bool attachValues) {
    d = new CallResponsePrivate();
    d->messageHeader = new MessageHeader(MessageHeader::CALL_RESPONSE,
            messageId);
    d->status = status;
    d->methodId = NULL;
    d->paramId = NULL;
    d->paramList = NULL;
    d->hasAttachedValues = attachValues;
}

CallResponse::CallResponse(const CallResponse& orig) {
    d = new CallResponsePrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->status = orig.d->status;
    d->methodId = orig.d->methodId == NULL ? NULL : new ParamId(*orig.d->methodId);
    d->paramId = orig.d->paramId == NULL ? NULL : new ParamId(*orig.d->paramId);
    d->paramList = orig.d->paramList == NULL ? NULL : new ParamList(*orig.d->paramList);
    d->hasAttachedValues = true;
}

CallResponse::~CallResponse() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        delete d->methodId;
        delete d->paramId;
        delete d->paramList;
    }
    delete d;
}

Status::Value CallResponse::getStatus() const {
    return d->status;
}

void CallResponse::setStatus(Status::Value status) {
    d->status = status;
}

const ParamId* CallResponse::getMethodId() const {
    return d->methodId;
}

void CallResponse::setMethodId(const ParamId* methodId) {
    if (d->hasAttachedValues) {
        delete d->methodId;
    }
    d->methodId = methodId;
}

const ParamId* CallResponse::getParamId() const {
    return d->paramId;
}

void CallResponse::setParamId(const ParamId* paramId) {
    if (d->hasAttachedValues) {
        delete d->paramId;
    }
    d->paramId = paramId;
}

const ParamList* CallResponse::getParamList() const {
    return d->paramList;
}

void CallResponse::setParamList(const ParamList* paramList) {
    if (d->hasAttachedValues) {
        delete d->paramList;
    }
    d->paramList = paramList;
}

MessageHeader& CallResponse::getMessageHeader() {
    return *d->messageHeader;
}

Message* CallResponse::copy() const {
    return new CallResponse(*this);
}
