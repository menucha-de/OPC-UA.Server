#ifndef BINARYSERVER_EVENTFIELD_H
#define BINARYSERVER_EVENTFIELD_H

#include <common/Exception.h>
#include <uanodeid.h> // UaNodeId
#include <uaqualifiedname.h> // UaQualifiedName
#include <uavariant.h> // UaVariant

namespace BinaryServerNamespace {

    class EventFieldPrivate;

    class EventField {
    public:
        EventField(const UaNodeId& nodeId, bool attachValues = false);
        EventField(const EventField& orig);
        virtual ~EventField();

        const UaNodeId& getNodeId() const;
        void setNodeId(const UaNodeId& nodeId);
        const UaQualifiedName* getQualifiedName() const;
        void setQualifiedName(const UaQualifiedName* qName);
        const UaNodeId* getDataTypeId() const;
        void setDataTypeId(const UaNodeId* dataTypeId);
        const UaVariant* getValue() const;
        void setValue(const UaVariant* value);

        virtual CommonNamespace::Exception* getException() const;
        virtual void setException(CommonNamespace::Exception* exception);
    private:
        EventField& operator=(const EventField&);

        EventFieldPrivate* d;
    };

} // BinaryServerNamespace
#endif /* BINARYSERVER_EVENTFIELD_H */

