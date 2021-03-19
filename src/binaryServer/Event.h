#ifndef BINARYSERVER_EVENT_H
#define BINARYSERVER_EVENT_H

#include "EventField.h"
#include <uanodeid.h> // UaNodeId
#include <uadatetime.h> // UaDateTime
#include <string>

namespace BinaryServerNamespace {

    class EventPrivate;

    class Event {
    public:
        Event(const UaNodeId& eventTypeId,
                std::vector<BinaryServerNamespace::EventField*>& eventFields,
                bool attachValues = false);
        virtual ~Event();

        virtual const UaNodeId& getEventTypeId() const;
        virtual void setEventTypeId(const UaNodeId& eventTypeId);
        virtual std::vector<BinaryServerNamespace::EventField*>* getEventFields() const;
        virtual void setEventFields(
                std::vector<BinaryServerNamespace::EventField*>* eventFields);

    private:
        Event(const Event& orig);
        Event& operator=(const Event&);

        EventPrivate* d;
    };

} // BinaryServerNamespace
#endif /* BINARYSERVER_EVENT_H */

