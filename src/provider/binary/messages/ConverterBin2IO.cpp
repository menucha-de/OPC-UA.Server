#include "ConverterBin2IO.h"
#include "../common/ConversionException.h"
#include "dto/Array.h"
#include "dto/Scalar.h"
#include "dto/Struct.h"
#include <common/Exception.h>
#include <common/ScopeGuard.h>
#include <common/MapScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <ioDataProvider/Array.h>
#include <ioDataProvider/Scalar.h>
#include <ioDataProvider/Structure.h>
#include <sstream> // ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

class ConverterBin2IOPrivate {
    friend class ConverterBin2IO;
private:
    ConverterBin2IO* parent;
    IODataProviderNamespace::Variant* convertBin2ioScalarValue(const Scalar& value)/* throws ConversionException */;
    IODataProviderNamespace::Variant* convertBin2ioArrayValue(const Array& value, int destNamespaceIndex);
    IODataProviderNamespace::Variant* convertBin2ioStructureValue(const Struct& value, int destNamespaceIndex);

    Variant* convertIo2binScalarValue(const IODataProviderNamespace::Scalar& value);
    Variant* convertIo2binNodeIdValue(const IODataProviderNamespace::Variant& value);
    Variant* convertIo2binArrayValue(const IODataProviderNamespace::Array& value);
    Variant* convertIo2binStructureValue(const IODataProviderNamespace::Structure& value);
};

ConverterBin2IO::ConverterBin2IO() {
    d = new ConverterBin2IOPrivate();
    d->parent = this;
}

ConverterBin2IO::~ConverterBin2IO() {
    delete d;
}

IODataProviderNamespace::NodeId * ConverterBin2IO::convertBin2io(const ParamId& paramId, int destNamespaceIndex)
/* throws ConversionException */ {
    int namespaceIndex = destNamespaceIndex;
    if (paramId.getNamespaceIndex() >= 0) {
        namespaceIndex = paramId.getNamespaceIndex();
    }
    switch (paramId.getParamIdType()) {
        case ParamId::NUMERIC:
            return new IODataProviderNamespace::NodeId(namespaceIndex, paramId.getNumeric());
        case ParamId::STRING:
            return new IODataProviderNamespace::NodeId(namespaceIndex,
                    *new std::string(paramId.getString()), true /* attachValues */);
        default:
            throw ExceptionDef(ConversionException,
                    std::string("Invalid paramId type ").append(paramId.toString()));
    }
}

ParamId* ConverterBin2IO::convertIo2bin(const IODataProviderNamespace::NodeId& nodeId)
/* throws ConversionException */ {
    switch (nodeId.getNodeType()) {
        case IODataProviderNamespace::NodeId::NUMERIC:
            return new ParamId(nodeId.getNamespaceIndex(), nodeId.getNumeric());
        case IODataProviderNamespace::NodeId::STRING:
            return new ParamId(nodeId.getNamespaceIndex(), *new std::string(nodeId.getString()), true /* attachValues */);
        default:
            throw ExceptionDef(ConversionException,
                    std::string("Invalid nodeId type ").append(nodeId.toString()));
    }
}

IODataProviderNamespace::Variant* ConverterBin2IO::convertBin2io(const Variant& value,
        int destNamespaceIndex)/* throws ConversionException */ {
    // Binary       - InternalInterface
    // boolean(8)   - bool(8)
    // byte(8)      - schar(8)
    // char(8)      - char(8)
    // short(16)    - int(16)
    // int(32)      - long(32)
    // long(64)     - llong(64)
    // float        - float
    // double       - double
    // char[]       - string
    // byte[]       - byteString
    switch (value.getVariantType()) {
        case Variant::SCALAR:
            return d->convertBin2ioScalarValue(static_cast<const Scalar&> (value));
        case Variant::ARRAY:
            return d->convertBin2ioArrayValue(static_cast<const Array&> (value),
                    destNamespaceIndex);
        case Variant::STRUCT:
            return d->convertBin2ioStructureValue(static_cast<const Struct&> (value),
                    destNamespaceIndex);
        default:
            throw ExceptionDef(ConversionException,
                    std::string("Unsupported variant type ").append(value.toString()));
    }
}

Variant* ConverterBin2IO::convertIo2bin(const IODataProviderNamespace::Variant& value)
/* throws ConversionException */ {
    // InternalInterface - Binary
    // bool(8)           - boolean(8)
    // schar(8)          - byte(8)
    // char(8)           - char(8)
    // int(16)           - short(16)
    // uint(16)          - int(32)
    // long(32)          - int(32)
    // ulong(32)         - long(64)
    // llong(64)         - long(64)
    // ullong(64)        - ERROR
    // float             - float
    // double            - double
    // string            - char[]
    // byteString        - byte[]
    // localizedText     - char[]
    
    switch (value.getVariantType()) {
        case IODataProviderNamespace::Variant::SCALAR:
            return d->convertIo2binScalarValue(
                    static_cast<const IODataProviderNamespace::Scalar&> (value));
        case IODataProviderNamespace::Variant::ARRAY:
            return d->convertIo2binArrayValue(
                    static_cast<const IODataProviderNamespace::Array&> (value));
        case IODataProviderNamespace::Variant::STRUCTURE:
            return d->convertIo2binStructureValue(
                    static_cast<const IODataProviderNamespace::Structure&> (value));
        case IODataProviderNamespace::Variant::NODE_ID:
            return d->convertIo2binNodeIdValue(value);
        case IODataProviderNamespace::Variant::NODE_PROPERTIES:
        case IODataProviderNamespace::Variant::OPC_UA_EVENT_DATA:
        default:
            throw ExceptionDef(ConversionException,
                    std::string("Unsupported variant type ").append(value.toString()));
    }
}




IODataProviderNamespace::Variant* ConverterBin2IOPrivate::convertBin2ioScalarValue(
        const Scalar& value)/* throws ConversionException */ {
    IODataProviderNamespace::Scalar* ret = new IODataProviderNamespace::Scalar();
    switch (value.getScalarType()) {
        case Scalar::BOOLEAN:
            ret->setBool(value.getBoolean());
            break;
        case Scalar::BYTE:
            ret->setSChar(value.getByte());
            break;
        case Scalar::CHAR:
            ret->setChar(value.getChar());
            break;
        case Scalar::SHORT:
            ret->setInt(value.getShort());
            break;
        case Scalar::INT:
            ret->setLong(value.getInt());
            break;
        case Scalar::LONG:
            ret->setLLong(value.getLong());
            break;
        case Scalar::FLOAT:
            ret->setFloat(value.getFloat());
            break;
        case Scalar::DOUBLE:
            ret->setDouble(value.getDouble());
            break;
        default:
            throw ExceptionDef(ConversionException,
                    std::string("Unsupported scalar type ").append(value.toString()));
    }
    return ret;
}

IODataProviderNamespace::Variant* ConverterBin2IOPrivate::convertBin2ioArrayValue(
        const Array& value, int destNamespaceIndex) /* throws ConversionException */ {
    const std::vector<const Variant*>& elements = value.getElements();
    switch (value.getArrayType()) {
        case Scalar::CHAR:
        {
            // convert char array to string
            char str[elements.size() + 1];
            int strPos = 0;
            // for each arrayValue
            for (std::vector<const Variant*>::const_iterator i =
                    elements.begin(); i != elements.end(); i++, strPos++) {
                const Scalar& element = static_cast<const Scalar&> (**i);
                str[strPos] = element.getChar();
            }
            str[strPos] = 0;
            IODataProviderNamespace::Scalar* s = new IODataProviderNamespace::Scalar();
            s->setString(new std::string(str), true /* attachValues */);
            return s;
        }
        case Scalar::BYTE:
        {
            // convert byte array to byteString
            char* str = new char[elements.size()];
            int strPos = 0;
            // for each arrayValue
            for (std::vector<const Variant*>::const_iterator i =
                    elements.begin(); i != elements.end(); i++, strPos++) {
                const Scalar& element = static_cast<const Scalar&> (**i);
                str[strPos] = element.getByte();
            }

            IODataProviderNamespace::Scalar* s = new IODataProviderNamespace::Scalar();
            s->setByteString(str, elements.size(), true /* attachValues */);
            return s;
        }
        default:
        {
            int arrayType;
            switch (value.getArrayType()) {
                case Scalar::BOOLEAN:
                    arrayType = IODataProviderNamespace::Scalar::BOOL;
                    break;
                case Scalar::SHORT:
                    arrayType = IODataProviderNamespace::Scalar::INT;
                    break;
                case Scalar::INT:
                    arrayType = IODataProviderNamespace::Scalar::LONG;
                    break;
                case Scalar::LONG:
                    arrayType = IODataProviderNamespace::Scalar::LLONG;
                    break;
                case Scalar::FLOAT:
                    arrayType = IODataProviderNamespace::Scalar::FLOAT;
                    break;
                case Scalar::DOUBLE:
                    arrayType = IODataProviderNamespace::Scalar::DOUBLE;
                    break;
                case Variant::ARRAY:
                    arrayType = IODataProviderNamespace::Variant::ARRAY;
                    break;
                case Variant::STRUCT:
                    arrayType = IODataProviderNamespace::Variant::STRUCTURE;
                    break;
                default:
                    std::ostringstream msg;
                    msg << "Unsupported array type " << value.getArrayType();
                    throw ExceptionDef(ConversionException, msg.str());
            }
            std::vector<const IODataProviderNamespace::Variant*>* elems =
                    new std::vector<const IODataProviderNamespace::Variant*>();
            VectorScopeGuard<const IODataProviderNamespace::Variant> elemsSG(elems);
            for (std::vector<const Variant*>::const_iterator i = elements.begin();
                    i != elements.end(); i++) {
                IODataProviderNamespace::Variant* value
                        = parent->convertBin2io(**i, destNamespaceIndex); // ConversionException
                // if nested array with scalar elements
                if (IODataProviderNamespace::Variant::ARRAY == arrayType
                        && IODataProviderNamespace::Variant::SCALAR == value->getVariantType()) {
                    IODataProviderNamespace::Scalar& scalar =
                            *static_cast<IODataProviderNamespace::Scalar*> (value);
                    // use scalar type as array type
                    arrayType = scalar.getScalarType();
                }
                elems->push_back(value);
            }
            return new IODataProviderNamespace::Array(arrayType, elemsSG.detach(),
                    true /*attachValues*/);
        }
    }
}

IODataProviderNamespace::Variant* ConverterBin2IOPrivate::convertBin2ioStructureValue(
        const Struct& value, int destNamespaceIndex) /* throws ConversionException */ {
    IODataProviderNamespace::NodeId* dataTypeId
            = parent->convertBin2io(value.getStructId(), destNamespaceIndex); // ConversionException
    ScopeGuard<IODataProviderNamespace::NodeId> dataTypeIdSG(dataTypeId);
    std::map<std::string, const IODataProviderNamespace::Variant*>* fieldData =
            new std::map<std::string, const IODataProviderNamespace::Variant*>();
    MapScopeGuard<std::string, const IODataProviderNamespace::Variant> fieldDataSG(fieldData);
    // for each field
    for (std::map<std::string, const Variant*>::const_iterator i = value.getFields().begin();
            i != value.getFields().end(); i++) {
        std::string fieldName = (*i).first;
        const Variant& fieldValue = *(*i).second;
        // convert field value
        (*fieldData)[fieldName] = parent->convertBin2io(fieldValue, destNamespaceIndex); // ConversionException
    }
    return new IODataProviderNamespace::Structure(*dataTypeIdSG.detach(), *fieldDataSG.detach(),
            true /*attachValues*/);
}

Variant* ConverterBin2IOPrivate::convertIo2binNodeIdValue(const IODataProviderNamespace::Variant& value)
{
    std::vector<const Variant*>* elements = new std::vector<const Variant*>();
    std::string copyStr(value.toString());
    const char* str =  copyStr.c_str();
    if (str != NULL) {
        for (size_t i = 0; i < copyStr.length(); i++) {
            Scalar* s = new Scalar();
            s->setChar(str[i]);
            elements->push_back(s);
        }
    }
    Array* array = new Array(Scalar::CHAR, *elements, true/* attachValues*/);
    return array;
}


Variant* ConverterBin2IOPrivate::convertIo2binScalarValue(
        const IODataProviderNamespace::Scalar& value)/* throws ConversionException */ {
    Scalar* scalar = NULL;
    Array* array = NULL;
    switch (value.getScalarType()) {
        case IODataProviderNamespace::Scalar::BOOL:
            scalar = new Scalar();
            scalar->setBoolean(value.getBool());
            break;
        case IODataProviderNamespace::Scalar::SCHAR:
            scalar = new Scalar();
            scalar->setByte(value.getSChar());
            break;
        case IODataProviderNamespace::Scalar::CHAR:
            scalar = new Scalar();
            scalar->setChar(value.getChar());
            break;
        case IODataProviderNamespace::Scalar::INT:
            scalar = new Scalar();
            scalar->setShort(value.getInt());
            break;
        case IODataProviderNamespace::Scalar::UINT:
            scalar = new Scalar();
            scalar->setInt(static_cast<long> (value.getUInt()));
            break;
        case IODataProviderNamespace::Scalar::LONG:
            scalar = new Scalar();
            scalar->setInt(value.getLong());
            break;
        case IODataProviderNamespace::Scalar::ULONG:
            scalar = new Scalar();
            scalar->setLong(static_cast<long long> (value.getULong()));
            break;
        case IODataProviderNamespace::Scalar::LLONG:
            scalar = new Scalar();
            scalar->setLong(value.getLLong());
            break;
        case IODataProviderNamespace::Scalar::FLOAT:
            scalar = new Scalar();
            scalar->setFloat(value.getFloat());
            break;
        case IODataProviderNamespace::Scalar::DOUBLE:
            scalar = new Scalar();
            scalar->setDouble(value.getDouble());
            break;
        case IODataProviderNamespace::Scalar::STRING:
        { // char[]
            // NULL is converted to an empty char array (cause: see byteString)
            std::vector<const Variant*>* elements = new std::vector<const Variant*>();
            const char* str = value.getString() == NULL ? NULL : value.getString()->c_str();
            if (str != NULL) {
                for (size_t i = 0; i < value.getString()->size(); i++) {
                    Scalar* s = new Scalar();
                    s->setChar(str[i]);
                    elements->push_back(s);
                }
            }
            array = new Array(Scalar::CHAR, *elements, true/* attachValues*/);
            break;
        }
        case IODataProviderNamespace::Scalar::BYTE_STRING:
        { // byte[]
            // if an empty byte array is set to an UaByteString instance then it is converted 
            // internally to NULL => NULL must be converted to an empty byte array here
            std::vector<const Variant*>* elements = new std::vector<const Variant*>();
            const char* str = value.getByteString();
            if (str != NULL) {
                long length = value.getByteStringLength();
                for (size_t i = 0; i < length; i++) {
                    Scalar* s = new Scalar();
                    s->setByte(str[i]);
                    elements->push_back(s);
                }
            }
            array = new Array(Scalar::BYTE, *elements, true/* attachValues*/);
            break;
        }
        case IODataProviderNamespace::Scalar::LOCALIZED_TEXT:
        { // char[]
            // NULL is converted to an empty char array (cause: see byteString)
            std::vector<const Variant*>* elements = new std::vector<const Variant*>();
            const char* str = value.getLocalizedTextText() == NULL ? NULL :
                    value.getLocalizedTextText()->c_str();
            if (str != NULL) {
                for (size_t i = 0; i < value.getLocalizedTextText()->size(); i++) {
                    Scalar* s = new Scalar();
                    s->setChar(str[i]);
                    elements->push_back(s);
                }
            }
            array = new Array(Scalar::CHAR, *elements, true/* attachValues*/);
            break;
        }
        case IODataProviderNamespace::Scalar::ULLONG:
        default:
            throw ExceptionDef(ConversionException,
                    std::string("Unsupported scalar type ").append(value.toString()));
    }
    if (array == NULL) {
        return scalar;
    } else {
        return array;
    }
}

Variant* ConverterBin2IOPrivate::convertIo2binArrayValue(const IODataProviderNamespace::Array& value)
/* throws ConversionException */ {
    if (value.getElements() == NULL) {
        throw ExceptionDef(ConversionException,
                std::string("A null array cannot be converted to Variant"));
    }
    int arrayType;
    switch (value.getArrayType()) {
        case IODataProviderNamespace::Scalar::BOOL:
            arrayType = Scalar::BOOLEAN;
            break;
        case IODataProviderNamespace::Scalar::SCHAR:
            arrayType = Scalar::BYTE;
            break;
        case IODataProviderNamespace::Scalar::INT:
            arrayType = Scalar::SHORT;
            break;
        case IODataProviderNamespace::Scalar::UINT:
            arrayType = Scalar::INT;
            break;
        case IODataProviderNamespace::Scalar::LONG:
            arrayType = Scalar::INT;
            break;
        case IODataProviderNamespace::Scalar::ULONG:
            arrayType = Scalar::LONG;
            break;
        case IODataProviderNamespace::Scalar::LLONG:
            arrayType = Scalar::LONG;
            break;
        case IODataProviderNamespace::Scalar::FLOAT:
            arrayType = Scalar::FLOAT;
            break;
        case IODataProviderNamespace::Scalar::DOUBLE:
            arrayType = Scalar::DOUBLE;
            break;
        case IODataProviderNamespace::Scalar::STRING:
        case IODataProviderNamespace::Scalar::BYTE_STRING:
        case IODataProviderNamespace::Scalar::LOCALIZED_TEXT:
        case IODataProviderNamespace::Scalar::VARIANT:
            arrayType = Scalar::ARRAY;
            break;
        case IODataProviderNamespace::Variant::STRUCTURE:
            arrayType = Variant::STRUCT;
            break;
        case IODataProviderNamespace::Scalar::CHAR:
        case IODataProviderNamespace::Scalar::ULLONG:
        case IODataProviderNamespace::Variant::NODE_PROPERTIES:
        case IODataProviderNamespace::Variant::OPC_UA_EVENT_DATA:
        default:
            std::ostringstream msg;
            msg << "Unsupported array type " << value.getArrayType();
            throw ExceptionDef(ConversionException, msg.str());
            break;
    }
    std::vector<const Variant*>* elements = new std::vector<const Variant*>();
    VectorScopeGuard<const Variant> elementsSG(elements);
    const std::vector<const IODataProviderNamespace::Variant*>* elems = value.getElements();
    // for each element
    for (int i = 0; i < elems->size(); i++) {
        // convert element
        Variant* value = parent->convertIo2bin(*elems->at(i)); // ConversionException
        elements->push_back(value);
    }
    return new Array(arrayType, *elementsSG.detach(), true /*attachValues*/);
}

Variant* ConverterBin2IOPrivate::convertIo2binStructureValue(
        const IODataProviderNamespace::Structure& value)/* throws ConversionException */ {
    ParamId* structId = parent->convertIo2bin(value.getDataTypeId()); // ConversionException
    ScopeGuard<ParamId> structIdSG(structId);
    std::map<std::string, const Variant*>* fieldData = new std::map<std::string, const Variant*>();
    MapScopeGuard<std::string, const Variant> fieldDataSG(fieldData);
    // for each field
    for (std::map<std::string, const IODataProviderNamespace::Variant*>::const_iterator i =
            value.getFieldData().begin(); i != value.getFieldData().end(); i++) {
        std::string fieldName = (*i).first;
        const IODataProviderNamespace::Variant& fieldValue = *(*i).second;
        // convert field value
        (*fieldData)[fieldName] = parent->convertIo2bin(fieldValue); // ConversionException
    }
    return new Struct(*structIdSG.detach(), *fieldDataSG.detach(), true /*attachValues*/);
}
