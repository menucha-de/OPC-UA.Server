#ifndef PROVIDER_BINARY_MESSAGES_DTO_WRITE_H
#define PROVIDER_BINARY_MESSAGES_DTO_WRITE_H

#include "Message.h"
#include "ParamId.h"
#include "Variant.h"

class WritePrivate;

class Write : public Message {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    Write(unsigned long messageId, const ParamId& paramId,
            const Variant& paramValue, bool attachValues = false);
    // Creates a deep copy of the instance.
    Write(const Write& orig);
    virtual ~Write();

    virtual const ParamId& getParamId() const; // 32 bit
    virtual void setParamId(const ParamId& paramId);

    virtual const Variant& getParamValue() const; // 16 bit
    virtual void setParamValue(const Variant& paramValue);

    // interface Message
    virtual MessageHeader& getMessageHeader();
    virtual Message* copy() const;
private:    
    Write& operator=(const Write& orig);
    
    WritePrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_READ_H */

