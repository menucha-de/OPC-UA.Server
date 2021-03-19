#ifndef BINARYSERVER_CACHEDCONVERTERCALLBACK_H
#define BINARYSERVER_CACHEDCONVERTERCALLBACK_H

#include <sasModelProvider/base/ConverterUa2IO.h>
#include <uanodeid.h> // UaNodeId
#include <uastructuredefinition.h> // UaStructureDefinition
#include <vector>

namespace SASModelProviderNamespace {

    class CachedConverterCallbackPrivate;

    // This class is thread safe.

    class CachedConverterCallback : public ConverterUa2IO::ConverterCallback {
    public:
        CachedConverterCallback(ConverterUa2IO::ConverterCallback& callback,
                bool attachValue) /* throws MutexException */;
        virtual ~CachedConverterCallback();

        void clear();
        void preload(const UaNodeId& typeId);

        // interface ConverterCallback
        virtual UaStructureDefinition getStructureDefinition(const UaNodeId& typeId)
        /* throws ConversionException */;
        virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& typeId)
        /* throws ConversionException */;
    private:
        CachedConverterCallback(const CachedConverterCallback& orig);
        CachedConverterCallback& operator=(const CachedConverterCallback&);
        
        CachedConverterCallbackPrivate* d;
    };
} // namespace SASModelProviderNamespace
#endif /* BINARYSERVER_CACHEDCONVERTERCALLBACK_H */

