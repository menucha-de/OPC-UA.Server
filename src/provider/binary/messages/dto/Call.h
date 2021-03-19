#ifndef PROVIDER_BINARY_MESSAGES_DTO_CALL_H
#define PROVIDER_BINARY_MESSAGES_DTO_CALL_H

#include "Message.h"
#include "MessageHeader.h"
#include "ParamId.h"
#include "ParamList.h"

class CallPrivate;

class Call : public Message {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    Call(unsigned long messageId, const ParamId& methodId, const ParamId& paramId,
            const ParamList& paramList, bool attachValues = false);
    // Creates a deep copy of the instance.
    Call(const Call& orig);    
    virtual ~Call();

    virtual const ParamId& getMethodId() const;
    virtual void setMethodId(const ParamId& methodId);

    virtual const ParamId& getParamId() const;
    virtual void setParamId(const ParamId& paramId);

    virtual const ParamList& getParamList() const;
    virtual void setParamList(const ParamList& paramList);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const;
private:    
    Call& operator=(const Call& orig);
    
    CallPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_CALL_H */

