#include <ioDataProvider/MethodData.h>
#include <stddef.h> // NULL
#include <stdio.h>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class MethodDataPrivate {
        friend class MethodData;
    private:
        const NodeId* objectNodeId;
        const NodeId* methodNodeId;
        const std::vector<const Variant*>* methodArgs;
        IODataProviderException* exception;
        bool hasAttachedValues;

        void deleteMethodArgs();
    };

    MethodData::MethodData(const NodeId& objectNodeId, const NodeId& methodNodeId,
            const std::vector<const Variant*>& methodArgs, bool attachValues) {
        d = new MethodDataPrivate();
        d->objectNodeId = &objectNodeId;
        d->methodNodeId = &methodNodeId;
        d->methodArgs = &methodArgs;
        d->exception = NULL;
        d->hasAttachedValues = attachValues;
    }

    MethodData::MethodData(const MethodData& methodData) {
        // avoid self-assignment
        if (this == &methodData) {
            return;
        }
        d = new MethodDataPrivate();
        d->hasAttachedValues = true;
        d->objectNodeId = new NodeId(*methodData.d->objectNodeId);
        d->methodNodeId = new NodeId(*methodData.d->methodNodeId);

        std::vector<const Variant*>* ma = new std::vector<const Variant*>();
        for (int i = 0; i < methodData.d->methodArgs->size(); i++) {
            ma->push_back((*methodData.d->methodArgs)[i]->copy());
        }
        d->methodArgs = ma;

        d->exception = methodData.d->exception == NULL ?
                NULL : static_cast<IODataProviderException*> (methodData.d->exception->copy());
    }

    MethodData::~MethodData() {
        if (d->hasAttachedValues) {
            delete d->objectNodeId;
            delete d->methodNodeId;
            d->deleteMethodArgs();
            delete d->exception;
        }
        delete d;
    }

    const NodeId& MethodData::getObjectNodeId() const {
        return *d->objectNodeId;
    }

    const NodeId& MethodData::getMethodNodeId() const {
        return *d->methodNodeId;
    }

    const std::vector<const Variant*>& MethodData::getMethodArguments() const {
        return *d->methodArgs;
    }

    void MethodData::setMethodArguments(const std::vector<const Variant*>& args) {
        if (d->hasAttachedValues) {
            d->deleteMethodArgs();
        }
        d->methodArgs = &args;
    }

    IODataProviderException* MethodData::getException() const {
        return d->exception;
    }

    void MethodData::setException(IODataProviderException* e) {
        if (d->hasAttachedValues) {
            delete d->exception;
        }
        d->exception = e;
    }

    void MethodDataPrivate::deleteMethodArgs() {
        for (int i = 0; i < methodArgs->size(); i++) {
            delete (*methodArgs)[i];
        }
        delete methodArgs;
    }

} // namespace IODataProviderNamespace
