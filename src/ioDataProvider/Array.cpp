#include <ioDataProvider/Array.h>
#include <sstream> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class ArrayPrivate {
        friend class Array;
    private:
        int arrayType;
        const std::vector<const Variant*>* elements;
        bool hasAttachedValues;
    };

    Array::Array(int arrayType, const std::vector<const Variant*>* elements, bool attachValues) {
        d = new ArrayPrivate();
        d->arrayType = arrayType;
        d->elements = elements;
        d->hasAttachedValues = attachValues;
    }

    Array::Array(const Array& array) {
        // avoid self-assignment
        if (this == &array) {
            return;
        }
        d = new ArrayPrivate();
        d->arrayType = array.d->arrayType;
        std::vector<const Variant*>* newElements = NULL;
        if (array.d->elements != NULL) {
            newElements = new std::vector<const Variant*>();
            for (std::vector<const Variant*>::const_iterator i =
                    array.d->elements->begin(); i != array.d->elements->end(); i++) {
                newElements->push_back((*i)->copy());
            }
        }
        d->elements = newElements;
        d->hasAttachedValues = true;
    }

    Array::~Array() {
        if (d->hasAttachedValues) {
            if (d->elements != NULL) {
                for (std::vector<const Variant*>::const_iterator i =
                        d->elements->begin(); i != d->elements->end(); i++) {
                    delete *i;
                }
                delete d->elements;
            }
        }
        delete d;
    }

    int Array::getArrayType() const {
        return d->arrayType;
    }

    const std::vector<const Variant*>* Array::getElements() const {
        return d->elements;
    }

    Variant* Array::copy() const {
        return new Array(*this);
    }

    Variant::Type Array::getVariantType() const {
        return ARRAY;
    }

    std::string Array::toString() const {
        std::ostringstream msg;
        msg << "IODataProviderNamespace::Array[type=" << d->arrayType << ",elements=";
        if (d->elements == NULL) {
            msg << "<NULL>";
        } else {
            for (std::vector<const Variant*>::const_iterator i = d->elements->begin();
                    i != d->elements->end(); i++) {
                if (i != d->elements->begin()) {
                    msg << ",";
                }
                msg << (*i)->toString();
            }
        }
        msg << "]";
        return msg.str();
    }

} // namespace IODataProviderNamespace
