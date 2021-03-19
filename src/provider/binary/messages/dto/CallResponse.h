#ifndef PROVIDER_BINARY_MESSAGES_DTO_CALLRESPONSE_H
#define PROVIDER_BINARY_MESSAGES_DTO_CALLRESPONSE_H

#include "Message.h"
#include "MessageHeader.h"
#include "ParamId.h"
#include "ParamList.h"
#include "Status.h"

class CallResponsePrivate;

class CallResponse : public Message {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    CallResponse(unsigned long messageId, Status::Value status, bool attachValues = false);
    // Creates a deep copy of the instance.
    CallResponse(const CallResponse& orig);    
    virtual ~CallResponse();

    virtual Status::Value getStatus() const; // 16 bit
    virtual void setStatus(Status::Value status);

    virtual const ParamId* getMethodId() const; // 16 bit
    virtual void setMethodId(const ParamId* methodId);

    virtual const ParamId* getParamId() const; // 16 bit
    virtual void setParamId(const ParamId* paramId);

    virtual const ParamList* getParamList() const;
    virtual void setParamList(const ParamList* paramValue);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const;
private:
    CallResponse& operator=(const CallResponse& orig);
    
    CallResponsePrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_CALLRESPONSE_H */

