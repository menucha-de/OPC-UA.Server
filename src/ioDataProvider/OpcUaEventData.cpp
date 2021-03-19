#include <ioDataProvider/OpcUaEventData.h>
#include <sstream> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class OpcUaEventDataPrivate {
        friend class OpcUaEventData;
    private:
        const NodeId* sourceNodeId;
        const std::string* message;
        int severity;
        const std::vector<const NodeData*>* fieldData;
        bool hasAttachedValues;
    };

    OpcUaEventData::OpcUaEventData(const NodeId& sourceNodeId,
            const std::string& message, int severity,
            const std::vector<const NodeData*>& fieldData, bool attachValue) {
        d = new OpcUaEventDataPrivate();
        d->sourceNodeId = &sourceNodeId;
        d->message = &message;
        d->severity = severity;
        d->fieldData = &fieldData;
        d->hasAttachedValues = attachValue;
    }

    OpcUaEventData::OpcUaEventData(const OpcUaEventData& eventData) {
        // avoid self-assignment
        if (this == &eventData) {
            return;
        }
        d = new OpcUaEventDataPrivate();
        d->hasAttachedValues = true;
        d->sourceNodeId = new NodeId(*eventData.d->sourceNodeId);
        d->message = new std::string(*eventData.d->message);
        d->severity = eventData.d->severity;

        std::vector<const NodeData*>* fd = new std::vector<const NodeData*>();
        for (int i = 0; i < eventData.d->fieldData->size(); i++) {
            fd->push_back(new NodeData(*(*eventData.d->fieldData)[i]));
        }
        d->fieldData = fd;
    }

    OpcUaEventData::~OpcUaEventData() {
        if (d->hasAttachedValues) {
            delete d->sourceNodeId;
            delete d->message;
            for (int i = 0; i < d->fieldData->size(); i++) {
                delete (*d->fieldData)[i];
            }
            delete d->fieldData;
        }
        delete d;
    }

    const NodeId& OpcUaEventData::getSourceNodeId() const {
        return *d->sourceNodeId;
    }

    const std::string& OpcUaEventData::getMessage() const {
        return *d->message;
    }

    int OpcUaEventData::getSeverity() const {
        return d->severity;
    }

    const std::vector<const NodeData*>& OpcUaEventData::getFieldData() const {
        return *d->fieldData;
    }

    Variant* OpcUaEventData::copy() const {
        return new OpcUaEventData(*this);
    }

    Variant::Type OpcUaEventData::getVariantType() const {
        return OPC_UA_EVENT_DATA;
    }

    std::string OpcUaEventData::toString() const {
        std::ostringstream msg;
        msg << "IODataProviderNamespace::OpcUaEventData[sourceNodeId="
                << d->sourceNodeId->toString() << ",message=" << *d->message << ",severity="
                << d->severity << ",fieldData=";
        for (int i = 0; i < d->fieldData->size(); i++) {
            if (i > 0) {
                msg << " ";
            }
            msg << (*d->fieldData)[i]->toString();
        }
        msg << "]";
        return msg.str();
    }

} // namespace IODataProviderNamespace
