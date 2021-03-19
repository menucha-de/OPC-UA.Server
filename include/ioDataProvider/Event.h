#ifndef IODATAPROVIDER_EVENT_H_
#define IODATAPROVIDER_EVENT_H_

#include "NodeData.h"
#include <ctime>
#include <vector>

namespace IODataProviderNamespace {

    class EventPrivate;

    class Event {
    public:
        // If a node identifier of an event type is used the relating data must be of type
        // "OpcUaEventData".
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the Event instance.
        Event(long long dateTime, const std::vector<const NodeData*>& nodeData,
                bool attachValues = false);
        // Creates a deep copy of the instance.
        Event(const Event& event);
        virtual ~Event();

        // time stamp in milliseconds since 01.01.1970
        const long long getDateTime() const;
        const std::vector<const NodeData*>& getNodeData() const;
    private:
        Event& operator=(const Event&);

        EventPrivate* d;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_EVENT_H_ */
