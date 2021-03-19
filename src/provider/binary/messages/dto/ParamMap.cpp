#include "ParamMap.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ParamMapPrivate {
    friend class ParamMap;
private:
    const std::map<const ParamId*, const Variant*>* elements;
    bool hasAttachedValues;

    void deleteElements();
};

ParamMap::ParamMap(const std::map<const ParamId*, const Variant*>& elements,
        bool attachValues) {
    d = new ParamMapPrivate();
    d->elements = &elements;
    d->hasAttachedValues = attachValues;
}

ParamMap::ParamMap(const ParamMap& orig) {
    d = new ParamMapPrivate();
    std::map<const ParamId*, const Variant*>* newElements = new std::map<
            const ParamId*, const Variant*>();
    for (std::map<const ParamId*, const Variant*>::const_iterator i =
            orig.d->elements->begin(); i != orig.d->elements->end(); i++) {
        (*newElements)[new ParamId(*(*i).first)] = (*i).second->copy();
    }
    d->elements = newElements;
    d->hasAttachedValues = true;
}

ParamMap::~ParamMap() {
    if (d->hasAttachedValues) {
        d->deleteElements();
    }
    delete d;
}

const std::map<const ParamId*, const Variant*>& ParamMap::getElements() const {
    return *d->elements;
}

void ParamMap::setElements(const std::map<const ParamId*, const Variant*>& elements) {
    if (d->hasAttachedValues) {
        d->deleteElements();
    }
    d->elements = &elements;
}

void ParamMapPrivate::deleteElements() {
    for (std::map<const ParamId*, const Variant*>::const_iterator i =
            elements->begin(); i != elements->end(); i++) {
        delete (*i).first;
        delete (*i).second;
    }
    delete elements;
}
