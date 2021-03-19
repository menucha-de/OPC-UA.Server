#include "ParamList.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ParamListPrivate {
    friend class ParamList;
private:
    const std::vector<const Variant*>* elements;
    bool hasAttachedValues;

    void deleteElements();
};

ParamList::ParamList(const std::vector<const Variant*>& elements,
        bool attachValues) {
    d = new ParamListPrivate();
    d->elements = &elements;
    d->hasAttachedValues = attachValues;
}

ParamList::ParamList(const ParamList& orig) {
    d = new ParamListPrivate();
    std::vector<const Variant*>* newElements =
            new std::vector<const Variant*>();
    for (std::vector<const Variant*>::const_iterator i =
            orig.d->elements->begin(); i != orig.d->elements->end(); i++) {
        newElements->push_back((*i)->copy());
    }
    d->elements = newElements;
    d->hasAttachedValues = true;
}

ParamList::~ParamList() {
    if (d->hasAttachedValues) {
        d->deleteElements();
    }
    delete d;
}

const std::vector<const Variant*>& ParamList::getElements() const {
    return *d->elements;
}

void ParamList::setElements(const std::vector<const Variant*>& elements) {
    if (d->hasAttachedValues) {
        d->deleteElements();
    }
    d->elements = &elements;
}

void ParamListPrivate::deleteElements() {
    for (std::vector<const Variant*>::const_iterator i = elements->begin();
            i != elements->end(); i++) {
        delete *i;
    }
    delete elements;
}
