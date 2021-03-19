#ifndef SASMODELPROVIDER_BASE_CONVERTER_H
#define SASMODELPROVIDER_BASE_CONVERTER_H

#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/Variant.h>
#include <uanodeid.h> // UaNodeId
#include <uastructuredefinition.h> // UaStructureDefinition
#include <uavariant.h> // UaVariant
#include <vector>

namespace SASModelProviderNamespace {

    class ConverterUa2IOPrivate;

    class ConverterUa2IO {
    public:

        class ConverterCallback {
        public:
            virtual UaStructureDefinition getStructureDefinition(const UaNodeId& dataTypeId) = 0;
            // The returned container must be deleted by the caller.
            virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& typeId) = 0;
        };

        // If the values are attached then the responsibility for destroying the callback instance
        // is delegated to the Converter instance.
        ConverterUa2IO(ConverterCallback& callback, bool attachValues = false);
        virtual ~ConverterUa2IO();

        // Converts a UaNodeId to a NodeId.
        // The returned NodeId instance must be destroyed by the caller.
        virtual IODataProviderNamespace::NodeId* convertUa2io(const UaNodeId& nodeId) const;
        // Converts a UaVariant to a Variant.
        // The returned Variant instance must be destroyed by the caller.
        virtual IODataProviderNamespace::Variant* convertUa2io(const UaVariant& value,
                const UaNodeId& srcDataTypeId);

        // Converts a NodeId to a UaNodeId.
        // The returned UaNodeId instance must be destroyed by the caller.
        virtual UaNodeId* convertIo2ua(const IODataProviderNamespace::NodeId& nodeId) const;
        // Converts a Variant to a UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        virtual UaVariant* convertIo2ua(const IODataProviderNamespace::Variant& value,
                const UaNodeId& destDataTypeId) const; /* throws ConversionException */
    private:
        ConverterUa2IO(const ConverterUa2IO& orig);
        ConverterUa2IO& operator=(const ConverterUa2IO&);

        ConverterUa2IOPrivate* d;
    };
} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_CONVERTER_H */

