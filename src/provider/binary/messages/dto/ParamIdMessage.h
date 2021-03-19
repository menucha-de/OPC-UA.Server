#ifndef PROVIDER_BINARY_MESSAGES_DTO_PARAMIDMESSAGE_H
#define PROVIDER_BINARY_MESSAGES_DTO_PARAMIDMESSAGE_H

#include "Message.h"
#include "MessageHeader.h"
#include "ParamId.h"

class ParamIdMessagePrivate;

class ParamIdMessage : public Message {
public:
    ParamIdMessage(MessageHeader::MessageType messageType,
            unsigned long messageId, const ParamId& paramId, bool attachValues =
            false);
    // Creates a deep copy of the instance.
    ParamIdMessage(const ParamIdMessage& orig);   
    virtual ~ParamIdMessage();

    virtual const ParamId& getParamId() const;
    virtual void setParamId(const ParamId& paramId);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const = 0;
private:
    ParamIdMessage& operator=(const ParamIdMessage& orig);
    
    ParamIdMessagePrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_PARAMIDMESSAGE_H */

