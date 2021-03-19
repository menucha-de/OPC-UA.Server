#ifndef PROVIDER_BINARY_MESSAGES_DTO_STRUCT_H
#define PROVIDER_BINARY_MESSAGES_DTO_STRUCT_H

#include "ParamId.h"
#include "Variant.h"
#include <map>
#include <string>

class StructPrivate;

class Struct : public Variant {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    Struct(const ParamId& structId, const std::map<std::string, const Variant*>& fields,
            bool attachValues = false);
    // Creates a deep copy of the instance.
    Struct(const Struct& orig);    
    virtual ~Struct();

    virtual const ParamId& getStructId() const;
    virtual void setStructId(const ParamId& structId);

    virtual const std::map<std::string, const Variant*>& getFields() const;
    virtual void setFields(const std::map<std::string, const Variant*>& fields);

    // interface Variant
    virtual Variant* copy() const;
    virtual Type getVariantType() const;
    virtual std::string toString() const;
private:
    Struct& operator=(const Struct& orig);
    
    StructPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_STRUCT_H */
