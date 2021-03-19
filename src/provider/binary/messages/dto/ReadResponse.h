#ifndef PROVIDER_BINARY_MESSAGES_DTO_READRESPONSE_H
#define PROVIDER_BINARY_MESSAGES_DTO_READRESPONSE_H

#include "Message.h"
#include "MessageHeader.h"
#include "ParamId.h"
#include "Status.h"
#include "Variant.h"

class ReadResponsePrivate;

class ReadResponse : public Message {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    ReadResponse(unsigned long messageId, Status::Value status,
            bool attachValues = false);
    // Creates a deep copy of the instance.
    ReadResponse(const ReadResponse& orig);    
    virtual ~ReadResponse();

    // 16 bit
    virtual Status::Value getStatus() const;
    virtual void setStatus(Status::Value status);

    virtual const ParamId* getParamId() const;
    virtual void setParamId(const ParamId* paramId);

    virtual const Variant* getParamValue() const;
    virtual void setParamValue(const Variant* paramValue);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const;
private:
    ReadResponse& operator=(const ReadResponse& orig);
    
    ReadResponsePrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_READRESPONSE_H */

