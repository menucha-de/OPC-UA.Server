#include <ioDataProvider/NodeData.h>
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class NodeDataPrivate {
        friend class NodeData;
    private:
        bool hasAttachedValues;
        const NodeId* nodeId;
        const Variant* data;
        IODataProviderException* exception;
    };

    NodeData::NodeData(const NodeId& nodeId, const Variant* data, bool attachValues) {
        d = new NodeDataPrivate();
        d->nodeId = &nodeId;
        d->data = data;
        d->exception = NULL;
        d->hasAttachedValues = attachValues;
    }

    NodeData::NodeData(const NodeData& nodeData) {
        // avoid self-assignment
        if (this == &nodeData) {
            return;
        }
        d = new NodeDataPrivate();
        d->hasAttachedValues = true;
        d->nodeId = new NodeId(*nodeData.d->nodeId);
        d->data = nodeData.d->data == NULL ? NULL : nodeData.d->data->copy();
        d->exception = nodeData.d->exception == NULL ?
                NULL : static_cast<IODataProviderException*> (nodeData.d->exception->copy());
    }

    NodeData::~NodeData() {
        if (d->hasAttachedValues) {
            delete d->nodeId;
            delete d->data;
            delete d->exception;
        }
        delete d;
    }

    const NodeId& NodeData::getNodeId() const {
        return *d->nodeId;
    }

    const Variant* NodeData::getData() const {
        return d->data;
    }

    void NodeData::setData(const Variant* data) {
        if (d->hasAttachedValues) {
            delete d->data;
        }
        d->data = data;
    }

    IODataProviderException* NodeData::getException() const {
        return d->exception;
    }

    void NodeData::setException(IODataProviderException* e) {
        if (d->hasAttachedValues) {
            delete d->exception;
        }
        d->exception = e;
    }

    std::string NodeData::toString() const {
        std::string st;
        if (d->exception != NULL) {
            d->exception->getStackTrace(st);
        }
        return std::string("IODataProviderNamespace::NodeData[nodeId=")
                .append(d->nodeId->toString())
                .append(",data=").append(d->data == NULL ? "<NULL>" : d->data->toString())
                .append(",exception=").append(d->exception == NULL ? "<NULL>" : st)
                .append("]");
    }

} // namespace IODataProviderNamespace
