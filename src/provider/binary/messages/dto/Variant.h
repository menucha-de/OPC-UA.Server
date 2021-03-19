#ifndef PROVIDER_BINARY_MESSAGES_DTO_VARIANT_H
#define PROVIDER_BINARY_MESSAGES_DTO_VARIANT_H

#include <string>

class Variant {
public:

    enum Type {
        SCALAR = -1, ARRAY = 8, STRUCT = 9
    };

    Variant();
    // Creates a deep copy of the Variant instance.
    // The returned instance must be destroyed by the caller.
    virtual Variant* copy() const = 0;
    virtual ~Variant();

    virtual Type getVariantType() const = 0;
    // Returns a string representation of the object (only for debugging purposes).
    virtual std::string toString() const = 0;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_VARIANT_H */
