#include "EventTypeData.h"
#include "common/Exception.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <map>
#include <sstream>

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

    class EventTypeDataPrivate {
        friend class EventTypeData;
    private:
        Logger* log;
        EventTypeRegistry* eventTypeRegistry;
        std::map<UaNodeId, UaVariant> fieldData;
    };

    EventTypeData::EventTypeData(const UaNodeId& eventTypeNodeId,
            EventTypeRegistry& eventTypeRegistry) {
        d = new EventTypeDataPrivate();
        d->log = LoggerFactory::getLogger("EventTypeData");
        m_EventTypeId.setNodeId(eventTypeNodeId.identifierNumeric(),
                eventTypeNodeId.namespaceIndex());
        d->eventTypeRegistry = &eventTypeRegistry;
    }

    EventTypeData::~EventTypeData() {
        delete d;
    }

    void EventTypeData::addFieldData(const UaNodeId& fieldNodeId,
            const UaVariant& data) {
        d->fieldData[fieldNodeId] = data;
    }

    void EventTypeData::getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data) {
        // get nodeId for field index from event field registry
        const UaNodeId* nodeId = d->eventTypeRegistry->getEventFieldNodeId(m_EventTypeId, index);
        if (nodeId == NULL) {
            BaseEventTypeData::getFieldData(index, pSession, data);
            return;
        }
        // get data for nodeId
        std::map<UaNodeId, UaVariant>::iterator fieldDataIter = d->fieldData.find(
                *nodeId);
        if (fieldDataIter == d->fieldData.end()) {
            std::ostringstream msg;
            msg << "Missing data for event field " << nodeId->toXmlString().toUtf8();
            Exception e = ExceptionDef(Exception, msg.str());
            // there is no way to inform the OPC UA server about details => log the exception
            std::string st;
            e.getStackTrace(st);
            d->log->error("Exception while getting field data: %s", st.c_str());
            return;
        }
        UaVariant& fieldData = fieldDataIter->second;
        // return data
        fieldData.copyTo(&data);
    }

} // namespace SASModelProviderNamespace
