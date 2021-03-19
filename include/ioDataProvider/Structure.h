#ifndef IODATAPROVIDER_STRUCTURE_H_
#define IODATAPROVIDER_STRUCTURE_H_

#include "NodeId.h"
#include "Variant.h"
#include <map>
#include <string>

namespace IODataProviderNamespace {

    class StructurePrivate;

    class Structure : public Variant {
    public:
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the Structure instance.
        // fieldData: field name => field value
        Structure(const NodeId& dataTypeId,
                const std::map<std::string, const Variant*>& fieldData, bool attachValues = false);
        // Creates a deep copy of the instance.
        Structure(const Structure& orig);
        virtual ~Structure();

        const NodeId& getDataTypeId() const;
        const std::map<std::string, const Variant*>& getFieldData() const;

        // interface Variant
        virtual Variant* copy() const;
        virtual Type getVariantType() const;
        virtual std::string toString() const;
    private:
        Structure& operator=(const Structure&);

        StructurePrivate* d;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_STRUCTURE_H_ */
