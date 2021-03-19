#include "ParamId.h"
#include <sstream> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ParamIdPrivate {
    friend class ParamId;
private:
    ParamId::Type paramIdType;
    int namespaceIndex;
    bool hasAttachedValues;

    union {
        unsigned long numericId;
        const std::string* stringId;
    };

	void setNamespaceIndex(int index);
    void setNumeric(long id);
    void setString(const std::string& id);
    void init(bool attachValues);
    void clear();
};

ParamId::ParamId(int namespaceIndex, unsigned long id) {
    d = new ParamIdPrivate();
    d->init(false /* attachValues */);
    d->setNamespaceIndex(namespaceIndex);
    d->setNumeric(id);
}

ParamId::ParamId(int namespaceIndex, const std::string& id, bool attachValues) {
    d = new ParamIdPrivate();
    d->init(attachValues);
    d->setNamespaceIndex(namespaceIndex);
    d->setString(id);
}

ParamId::ParamId(const ParamId& orig) {
    d = new ParamIdPrivate();
    d->paramIdType = orig.d->paramIdType;
    d->hasAttachedValues = true;
    d->namespaceIndex = orig.d->namespaceIndex;
    switch (d->paramIdType) {
        case NUMERIC:
            d->numericId = orig.d->numericId;
            break;
        case STRING:
            d->stringId = new std::string(*orig.d->stringId);
            break;
    }
}

ParamId::~ParamId() {
    d->clear();
    delete d;
}

ParamId::Type ParamId::getParamIdType() const {
    return d->paramIdType;
}

int ParamId::getNamespaceIndex() const {
    return d->namespaceIndex;
}

unsigned long ParamId::getNumeric() const {
    return d->numericId;
}

const std::string& ParamId::getString() const {
    return *d->stringId;
}

std::string ParamId::toString() const {
    std::ostringstream msg;
    msg << "ParamId[namespaceIndex=" << d->namespaceIndex;
    msg << ",type=";
    switch (d->paramIdType) {
        case NUMERIC:
            msg << "numeric,value=" << d->numericId;
            break;
        case STRING:
            msg << "string,value=" << d->stringId->c_str();
            break;
    }
    msg << "]";
    return msg.str();
}

void ParamIdPrivate::setNamespaceIndex(int index) {
    namespaceIndex = index;
}

void ParamIdPrivate::setNumeric(long id) {
    clear();
    paramIdType = ParamId::NUMERIC;
    numericId = id;
}

void ParamIdPrivate::setString(const std::string& id) {
    clear();
    paramIdType = ParamId::STRING;
    stringId = &id;
}

void ParamIdPrivate::init(bool attachValues) {
    paramIdType = ParamId::NUMERIC;
    hasAttachedValues = attachValues;
}

void ParamIdPrivate::clear() {
    if (hasAttachedValues) {
        switch (paramIdType) {
            case ParamId::NUMERIC:
                break;
            case ParamId::STRING:
                delete stringId;
                break;
        }
    }
}
