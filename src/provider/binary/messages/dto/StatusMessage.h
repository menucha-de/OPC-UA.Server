#ifndef PROVIDER_BINARY_MESSAGES_DTO_STATUSRESPONSE_H
#define PROVIDER_BINARY_MESSAGES_DTO_STATUSRESPONSE_H

#include "Message.h"
#include "Status.h"

class StatusMessagePrivate;

class StatusMessage : public Message {
public:
    StatusMessage(MessageHeader::MessageType messageType,
            unsigned long messageId, Status::Value status);
    // Creates a deep copy of the instance.
    StatusMessage(const StatusMessage& orig);
    virtual ~StatusMessage();

    virtual Status::Value getStatus() const; // 16 bit
    virtual void setStatus(Status::Value status);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const = 0;
private:    
    StatusMessage& operator=(const StatusMessage& orig);
    
    StatusMessagePrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_STATUSRESPONSE_H */

