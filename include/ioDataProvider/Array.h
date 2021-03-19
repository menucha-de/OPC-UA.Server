#ifndef IODATAPROVIDER_ARRAY_H_
#define IODATAPROVIDER_ARRAY_H_

#include "Variant.h"
#include <string>
#include <vector>

namespace IODataProviderNamespace {

    class ArrayPrivate;

    class Array : public Variant {
    public:
        // An array may be NULL outside of type Variant (see OpcUa Part 6 5.2.4).
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the Array instance.
        Array(int arrayType, const std::vector<const Variant*>* elements,
                bool attachValues = false);
        // Creates a deep copy of the instance.
        Array(const Array& array);
        virtual ~Array();

        int getArrayType() const;
        const std::vector<const Variant*>* getElements() const;

        // interface Variant
        virtual Variant* copy() const;
        virtual Variant::Type getVariantType() const;
        virtual std::string toString() const;
    private:
        Array& operator=(const Array&);

        ArrayPrivate* d;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_ARRAY_H_ */
