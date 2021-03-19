#ifndef PROVIDER_BINARY_MESSAGES_DTO_ARRAY_H
#define PROVIDER_BINARY_MESSAGES_DTO_ARRAY_H

#include "Scalar.h"
#include "Variant.h"
#include <vector>

class ArrayPrivate;

class Array : public Variant {
public:
    // If the values are attached then the responsibility for destroying the value instances
    // is delegated to the instance.
    Array(int arrayType, const std::vector<const Variant*>& elements,
            bool attachValues = false);
    // Creates a deep copy of the instance.
    Array(const Array& array);
    virtual ~Array();

    virtual int getArrayType() const;
    virtual void setArrayType(int arrayType);

    virtual const std::vector<const Variant*>& getElements() const;
    virtual void setElements(const std::vector<const Variant*>& elements);

    // interface Variant
    virtual Variant* copy() const;
    virtual Type getVariantType() const;
    virtual std::string toString() const;
private:
    Array& operator=(const Array& orig);

    ArrayPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_ARRAY_H */
