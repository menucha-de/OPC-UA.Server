#include "Event.h"
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class EventPrivate {
    friend class Event;
private:
    MessageHeader* messageHeader;
    const ParamId* eventId;
    const ParamId* paramId;
    long long timeStamp;
    long severity;
    const std::string* message;
    const ParamMap* paramMap;
    bool hasAttachedValues;
};

Event::Event(unsigned long messageId, const ParamId& eventId, const ParamId& paramId,
        long long timeStamp, long severity, const std::string& message,
        const ParamMap& paramMap, bool attachValues) {
    d = new EventPrivate();
    d->messageHeader = new MessageHeader(MessageHeader::EVENT, messageId);
    d->eventId = &eventId;
    d->paramId = &paramId;
    d->timeStamp = timeStamp;
    d->severity = severity;
    d->message = &message;
    d->paramMap = &paramMap;
    d->hasAttachedValues = attachValues;
}

Event::Event(const Event& orig) {
    d = new EventPrivate();
    d->messageHeader = new MessageHeader(*orig.d->messageHeader);
    d->eventId = new ParamId(*orig.d->eventId);
    d->paramId = new ParamId(*orig.d->paramId);
    d->timeStamp = orig.d->timeStamp;
    d->severity = orig.d->severity;
    d->message = new std::string(*orig.d->message);
    d->paramMap = new ParamMap(*orig.d->paramMap);
    d->hasAttachedValues = true;
}

Event::~Event() {
    delete d->messageHeader;
    if (d->hasAttachedValues) {
        delete d->eventId;
        delete d->paramId;
        delete d->message;
        delete d->paramMap;
    }
    delete d;
}

const ParamId& Event::getEventId() const {
    return *d->eventId;
}

void Event::setEventId(const ParamId& eventId) {
    if (d->hasAttachedValues) {
        delete d->eventId;
    }
    this->d->eventId = &eventId;
}

const ParamId& Event::getParamId() const {
    return *d->paramId;
}

void Event::setParamId(const ParamId& paramId) {
    if (d->hasAttachedValues) {
        delete d->paramId;
    }
    d->paramId = &paramId;
}

long long Event::getTimeStamp() const {
    return d->timeStamp;
}

void Event::setTimeStamp(long long timeStamp) {
    d->timeStamp = timeStamp;
}

long Event::getSeverity() const {
    return d->severity;
}

void Event::setSeverity(long severity) {
    d->severity = severity;
}

const std::string& Event::getMessage() const {
    return *d->message;
}

void Event::setMessage(const std::string& message) {
    if (d->hasAttachedValues) {
        delete d->message;
    }
    d->message = &message;
}

const ParamMap& Event::getParamMap() const {
    return *d->paramMap;
}

void Event::setParamMap(const ParamMap& paramMap) {
    if (d->hasAttachedValues) {
        delete d->paramMap;
    }
    d->paramMap = &paramMap;
}

MessageHeader& Event::getMessageHeader() {
    return *d->messageHeader;
}

Message* Event::copy() const {
    return new Event(*this);
}
