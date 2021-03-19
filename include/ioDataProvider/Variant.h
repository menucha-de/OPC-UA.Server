#ifndef IODATAPROVIDER_VARIANT_H_
#define IODATAPROVIDER_VARIANT_H_

#include <string>

namespace IODataProviderNamespace {

    class Variant {
    public:

        enum Type {
            SCALAR = -1, ARRAY = 30, NODE_ID = 31, OPC_UA_EVENT_DATA = 34, STRUCTURE = 32, NODE_PROPERTIES = 33
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

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_VARIANT_H_ */
