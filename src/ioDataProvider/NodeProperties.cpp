#include <ioDataProvider/NodeProperties.h>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class NodePropertiesPrivate {
        friend class NodeProperties;
    private:
        NodeProperties::ValueHandling valueHandling;
    };

    NodeProperties::NodeProperties(ValueHandling valueHandling) {
        d = new NodePropertiesPrivate();
        d->valueHandling = valueHandling;
    }

    NodeProperties::NodeProperties(const NodeProperties& nodeProperties) {
        // avoid self-assignment
        if (this == &nodeProperties) {
            return;
        }
        d = new NodePropertiesPrivate();
        d->valueHandling = nodeProperties.d->valueHandling;
    }

    NodeProperties::~NodeProperties() {
        delete d;
    }

    NodeProperties::ValueHandling NodeProperties::getValueHandling() const {
        return d->valueHandling;
    }

    void NodeProperties::setValueHandling(ValueHandling valueHandling) {
        d->valueHandling = valueHandling;
    }

    Variant* NodeProperties::copy() const {
        return new NodeProperties(*this);
    }

    Variant::Type NodeProperties::getVariantType() const {
        return NODE_PROPERTIES;
    }

    std::string NodeProperties::toString() const {
        std::string ret("IODataProviderNamespace::NodeProperties[valueHandling=");
        switch (d->valueHandling) {
            case NONE:
                ret.append("none");
                break;
            case SYNC:
                ret.append("sync");
                break;
            case ASYNC:
                ret.append("async");
                break;
        }
        return ret.append("]");
    }

} // namespace IODataProviderNamespace
