#ifndef PROVIDER_BINARY_MESSAGES_DTO_NOTIFICATION_H
#define PROVIDER_BINARY_MESSAGES_DTO_NOTIFICATION_H

#include "Message.h"
#include "MessageHeader.h"
#include "ParamMap.h"

class NotificationPrivate;

class Notification : public Message {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    Notification(unsigned long messageId, const ParamMap& paramMap,
            bool attachValues = false);
    // Creates a deep copy of the instance.
    Notification(const Notification& orig);
    virtual ~Notification();

    virtual const ParamMap& getParamMap() const;
    virtual void setParamMap(const ParamMap& paramMap);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const;
private:
    Notification& operator=(const Notification& orig);

    NotificationPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_NOTIFICATION_H */

