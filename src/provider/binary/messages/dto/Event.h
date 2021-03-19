#ifndef PROVIDER_BINARY_MESSAGES_DTO_EVENT_H
#define PROVIDER_BINARY_MESSAGES_DTO_EVENT_H

#include "Message.h"
#include "MessageHeader.h"
#include "ParamId.h"
#include "ParamMap.h"
#include <ctime> // std::time_t

class EventPrivate;

class Event : public Message {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    Event(unsigned long messageId, const ParamId& eventId, const ParamId& paramId,
            long long timeStamp, long severity, const std::string& message,
            const ParamMap& paramMap, bool attachValues = false);
    // Creates a deep copy of the instance.
    Event(const Event& orig);
    virtual ~Event();

    virtual const ParamId& getEventId() const;
    virtual void setEventId(const ParamId& eventId);

    virtual const ParamId& getParamId() const;
    virtual void setParamId(const ParamId& paramId);

    // time stamp in milliseconds since 01.01.1970
    virtual long long getTimeStamp() const;
    virtual void setTimeStamp(long long timeStamp);

    virtual long getSeverity() const;
    virtual void setSeverity(long severity);

    virtual const std::string& getMessage() const;
    virtual void setMessage(const std::string& message);

    virtual const ParamMap& getParamMap() const;
    virtual void setParamMap(const ParamMap& paramMap);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const;
private:
    Event& operator=(const Event& orig);

    EventPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_EVENT_H */

