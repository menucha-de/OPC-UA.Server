#include <ioDataProvider/Event.h>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class EventPrivate {
        friend class Event;
    private:
        long long dateTime;
        const std::vector<const NodeData*>* nodeData;
        bool hasAttachedValues;
    };

    Event::Event(long long dateTime, const std::vector<const NodeData*>& nodeData, bool attachValues) {
        d = new EventPrivate();
        d->dateTime = dateTime;
        d->nodeData = &nodeData;
        d->hasAttachedValues = attachValues;
    }

    Event::Event(const Event& event) {
        // avoid self-assignment
        if (this == &event) {
            return;
        }
        d = new EventPrivate();
        d->dateTime = event.d->dateTime;
        std::vector<const NodeData*>* nd = new std::vector<const NodeData*>();
        for (std::vector<const NodeData*>::const_iterator i =
                event.d->nodeData->begin(); i != event.d->nodeData->end(); i++) {
            nd->push_back(new NodeData(**i));
        }
        d->nodeData = nd;
        d->hasAttachedValues = true;
    }

    Event::~Event() {
        if (d->hasAttachedValues) {
            for (std::vector<const NodeData*>::const_iterator i =
                    d->nodeData->begin(); i != d->nodeData->end(); i++) {
                delete *i;
            }
            delete d->nodeData;
        }
        delete d;
    }

    const long long Event::getDateTime() const {
        return d->dateTime;
    }

    const std::vector<const NodeData*>& Event::getNodeData() const {
        return *d->nodeData;
    }

} // namespace IODataProviderNamespace
