#ifndef PROVIDER_BINARY_IODATAPROVIDER_CONVERTER_H
#define PROVIDER_BINARY_IODATAPROVIDER_CONVERTER_H

#include "../messages/dto/ParamId.h"
#include "../messages/dto/Variant.h"
#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/Variant.h>

class ConverterBin2IOPrivate;

// This class is thread safe.

class ConverterBin2IO {
public:
    ConverterBin2IO();
    virtual ~ConverterBin2IO();

    virtual IODataProviderNamespace::NodeId* convertBin2io(const ParamId& paramId, int destNamespaceIndex);
    virtual IODataProviderNamespace::Variant* convertBin2io(const Variant& value, int destNamespaceIndex);

    virtual ParamId* convertIo2bin(const IODataProviderNamespace::NodeId& nodeId);
    virtual Variant* convertIo2bin(const IODataProviderNamespace::Variant& value);

private:
    ConverterBin2IO(const ConverterBin2IO& orig);
    ConverterBin2IO& operator=(const ConverterBin2IO&);

    ConverterBin2IOPrivate* d;
};

#endif /* PROVIDER_BINARY_IODATAPROVIDER_CONVERTER_H */

