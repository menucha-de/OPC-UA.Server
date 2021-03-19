#include <ioDataProvider/NodeId.h>
#include <sstream> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class NodeIdPrivate {
        friend class NodeId;
    private:
        int namespaceIndex;
        NodeId::Type nodeType;
        bool hasAttachedValues;

        union {
            long numericId;
            const std::string* stringId;
        };

        void setNumeric(int namespaceIndex, long id);
        void setString(int namespaceIndex, const std::string& id);
        void init(bool attachValues);
        void clear();
    };

    NodeId::NodeId(int namespaceIndex, long id) {
        d = new NodeIdPrivate();
        d->init(false /* attachValues */);
        d->setNumeric(namespaceIndex, id);
    }

    NodeId::NodeId(int namespaceIndex, const std::string& id, bool attachValues) {
        d = new NodeIdPrivate();
        d->init(attachValues);
        d->setString(namespaceIndex, id);
    }

    NodeId::NodeId(const NodeId& nodeId) {
        // avoid self-assignment
        if (this == &nodeId) {
            return;
        }
        d = new NodeIdPrivate();
        d->namespaceIndex = nodeId.d->namespaceIndex;
        d->nodeType = nodeId.d->nodeType;
        d->hasAttachedValues = true;
        switch (d->nodeType) {
            case NUMERIC:
                d->numericId = nodeId.d->numericId;
                break;
            case STRING:
                d->stringId = new std::string(*nodeId.d->stringId);
                break;
        }
    }

    NodeId::~NodeId() {
        d->clear();
        delete d;
    }

    bool NodeId::equals(const NodeId& nodeId) const {
        return d->namespaceIndex == nodeId.d->namespaceIndex
                && d->nodeType == nodeId.d->nodeType
                && (d->nodeType == NUMERIC ?
                d->numericId == nodeId.d->numericId :
                *d->stringId == *nodeId.d->stringId);
    }

    NodeId::Type NodeId::getNodeType() const {
        return d->nodeType;
    }

    int NodeId::getNamespaceIndex() const {
        return d->namespaceIndex;
    }

    long NodeId::getNumeric() const {
        return d->numericId;
    }

    const std::string& NodeId::getString() const {
        return *d->stringId;
    }

    Variant* NodeId::copy() const {
        return new NodeId(*this);
    }

    Variant::Type NodeId::getVariantType() const {
        return NODE_ID;
    }

    std::string NodeId::toString() const {
        std::ostringstream msg;
        msg << "IODataProviderNamespace::NodeId[type=";
        switch (d->nodeType) {
            case NodeId::NUMERIC:
                msg << "numeric,ns=" << d->namespaceIndex << ",value=" << d->numericId;
                break;
            case NodeId::STRING:
                msg << "string,ns=" << d->namespaceIndex << ",value=" << d->stringId->c_str();
                break;
        }
        msg << "]";
        return msg.str();
    }

    void NodeIdPrivate::setNumeric(int namespaceIndex, long id) {
        clear();
        this->namespaceIndex = namespaceIndex;
        nodeType = NodeId::NUMERIC;
        numericId = id;
    }

    void NodeIdPrivate::setString(int namespaceIndex, const std::string& id) {
        clear();
        this->namespaceIndex = namespaceIndex;
        nodeType = NodeId::STRING;
        stringId = &id;
    }

    void NodeIdPrivate::init(bool attachValues) {
        nodeType = NodeId::NUMERIC;
        hasAttachedValues = attachValues;
    }

    void NodeIdPrivate::clear() {
        if (hasAttachedValues) {
            switch (nodeType) {
                case NodeId::STRING:
                    delete stringId;
                    break;
                default:
                    break;
            }
        }
    }

} // namespace IODataProviderNamespace
