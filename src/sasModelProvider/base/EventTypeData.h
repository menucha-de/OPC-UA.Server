#ifndef SASMODELPROVIDER_EVENTTYPEDATA_H
#define SASMODELPROVIDER_EVENTTYPEDATA_H

#include <sasModelProvider/base/EventTypeRegistry.h>
#include <session.h> // Session
#include <uaeventdata.h> // BaseEventTypeData
#include <uanodeid.h> // UaNodeId
#include <uavariant.h> // UaVariant

namespace SASModelProviderNamespace {

    class EventTypeDataPrivate;

    class EventTypeData : public BaseEventTypeData {
    public:
        EventTypeData(const UaNodeId& eventTypeNodeId, EventTypeRegistry& eventTypeRegistry);
        virtual ~EventTypeData();

        // Adds data for a field.
        // Copies of the parameter values are stored internally.
        virtual void addFieldData(const UaNodeId& fieldNodeId,
                const UaVariant& data);

        // interface BaseEventTypeData
        virtual void getFieldData(OpcUa_UInt32 index, Session* pSession,
                OpcUa_Variant& data);
    private:
        EventTypeDataPrivate* d;
    };

} // namespace SASModelProviderNamespace
#endif // SASMODELPROVIDER_EVENTTYPEDATA_H
