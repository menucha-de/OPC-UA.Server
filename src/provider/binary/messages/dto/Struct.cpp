#include "Struct.h"
#include <sstream> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class StructPrivate {
    friend class Struct;
private:
    const ParamId* structId;
    const std::map<std::string, const Variant*>* fields;
    bool hasAttachedValues;

    void deleteFields();
};

Struct::Struct(const ParamId& structId, const std::map<std::string, const Variant*>& fields,
        bool attachValues) {
    d = new StructPrivate();
    d->fields = &fields;
    d->structId = &structId;
    d->hasAttachedValues = attachValues;
}

Struct::Struct(const Struct& orig) {
    d = new StructPrivate();
    d->structId = new ParamId(*orig.d->structId);
    std::map<std::string, const Variant*>* newFields = new std::map<std::string, const Variant*>();
    for (std::map<std::string, const Variant*>::const_iterator i =
            orig.d->fields->begin(); i != orig.d->fields->end(); i++) {
        newFields->insert(
                std::pair<std::string, const Variant*>(
                std::string((*i).first), (*i).second->copy()));
    }
    d->fields = newFields;
    d->hasAttachedValues = true;
}

Struct::~Struct() {
    if (d->hasAttachedValues) {
        delete d->structId;
        d->deleteFields();
    }
    delete d;
}

const ParamId& Struct::getStructId() const {
    return *d->structId;
}

void Struct::setStructId(const ParamId& structId) {
    if (d->hasAttachedValues) {
        delete d->structId;
    }
    d->structId = &structId;
}

const std::map<std::string, const Variant*>& Struct::getFields() const {
    return *d->fields;
}

void Struct::setFields(const std::map<std::string, const Variant*>& fields) {
    if (d->hasAttachedValues) {
        d->deleteFields();
    }
    d->fields = &fields;
}

Variant* Struct::copy() const {
    return new Struct(*this);
}

Variant::Type Struct::getVariantType() const {
    return STRUCT;
}

std::string Struct::toString() const {
    std::ostringstream msg;
    msg << "Struct[structId=" << d->structId->toString() << ",fields=";
    for (std::map<std::string, const Variant*>::const_iterator i = d->fields->begin();
            i != d->fields->end(); i++) {
        if (i != d->fields->begin()) {
            msg << ",";
        }
        msg << (*i).first << "=>" << (*i).second->toString();
    }
    msg << "]";
    return msg.str();
}

void StructPrivate::deleteFields() {
    for (std::map<std::string, const Variant*>::const_iterator i =
            fields->begin(); i != fields->end(); i++) {
        delete (*i).second;
    }
    delete fields;
}
