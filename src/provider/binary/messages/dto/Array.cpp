#include "Array.h"
#include <sstream> // std::ostringstream
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ArrayPrivate {
    friend class Array;
private:
    int arrayType;
    const std::vector<const Variant*>* elements;
    bool hasAttachedValues;

    void deleteElements();
};

Array::Array(int arrayType, const std::vector<const Variant*>& elements, bool attachValues) {
    d = new ArrayPrivate();
    d->arrayType = arrayType;
    d->elements = &elements;
    d->hasAttachedValues = attachValues;
}

Array::Array(const Array& orig) {
    d = new ArrayPrivate();
    d->arrayType = orig.d->arrayType;
    std::vector<const Variant*>* newElements =
            new std::vector<const Variant*>();
    for (std::vector<const Variant*>::const_iterator i =
            orig.d->elements->begin(); i != orig.d->elements->end(); i++) {
        newElements->push_back((*i)->copy());
    }
    d->elements = newElements;
    d->hasAttachedValues = true;
}

Array::~Array() {
    if (d->hasAttachedValues) {
        d->deleteElements();
    }
    delete d;
}

int Array::getArrayType() const {
    return d->arrayType;
}

void Array::setArrayType(int arrayType) {
    d->arrayType = arrayType;
}

const std::vector<const Variant*>& Array::getElements() const {
    return *d->elements;
}

void Array::setElements(const std::vector<const Variant*>& elements) {
    if (d->hasAttachedValues) {
        d->deleteElements();
    }
    d->elements = &elements;
}

Variant* Array::copy() const {
    return new Array(*this);
}

Variant::Type Array::getVariantType() const {
    return ARRAY;
}

std::string Array::toString() const {
    std::ostringstream msg;
    msg << "Array[type=" << d->arrayType << ",elements=";
    for (std::vector<const Variant*>::const_iterator i = d->elements->begin();
            i != d->elements->end(); i++) {
        if (i != d->elements->begin()) {
            msg << ",";
        }
        msg << (*i)->toString();
    }
    msg << "]";
    return msg.str();
}

void ArrayPrivate::deleteElements() {
    for (std::vector<const Variant*>::const_iterator i = elements->begin();
            i != elements->end(); i++) {
        delete *i;
    }
    delete elements;
}
