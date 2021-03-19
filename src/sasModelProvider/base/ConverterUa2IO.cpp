#include <sasModelProvider/base/ConverterUa2IO.h>
#include <common/Exception.h> // ExceptionDef
#include <common/ScopeGuard.h>
#include <common/MapScopeGuard.h>
#include <common/VectorScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/Array.h>
#include <ioDataProvider/Scalar.h>
#include <ioDataProvider/Structure.h>
#include <sasModelProvider/base/ConversionException.h>
#include <uabytestring.h> // UaByteString
#include <uadatetime.h> // UaDateTime
#include <uagenericunionvalue.h> // UaGenericUnionValue
#include <sstream> // std::ostringstream
#include <stddef.h> // NULL
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;
using namespace IODataProviderNamespace;

namespace SASModelProviderNamespace {

    class ConverterUa2IOPrivate {
        friend class ConverterUa2IO;
    private:
        Logger* log;

        ConverterUa2IO* parent;
        ConverterUa2IO::ConverterCallback* callback;
        bool hasAttachedValues;

        UaNodeId getBuildInType(const UaNodeId& typeId) /* throws ConversionException */;

        // Converts an UaVariant to a Variant.
        // The returned Variant instance must be destroyed by the caller.
        Variant* convertUa2io(const UaVariant& value,
                const UaNodeId& srcDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts a value of a simple OPC UA type to a Variant
        // The returned Variant instance must be destroyed by the caller.
        Variant* convertUaBuildInType2io(const UaVariant& value, const UaNodeId& srcDataTypeId,
                OpcUa_Int16 indent);
        // Converts an UaExtensionObject to a Variant.
        // The returned Variant instance must be destroyed by the caller.
        Variant* convertUaExtensionObject2io(const UaExtensionObject& value,
                const UaNodeId& srcDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an UaVariant value of a UaStructureField to a Variant.
        // The returned Variant instance must be destroyed by the caller.
        Variant* convertUaStructureField2io(const UaStructureField& field,
                const UaVariant& value, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an array of type UaVariant to a Variant.
        // The returned Variant instance must be destroyed by the caller.
        Array* convertUaArray2io(const UaVariant& value,
                const UaNodeId& srcDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        int getIoArrayType(const UaNodeId dataTypeId) /* throws ConversionException */;

        // Converts a Variant to an UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIo2ua(const Variant& value,
                const UaNodeId& destDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts a Scalar to an UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoScalar2ua(const IODataProviderNamespace::Scalar& value,
                const UaNodeId& destDataTypeId, OpcUa_Int16 indent) const /* throws ConversionException */;
        // Converts a Structure to an UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoStructure2ua(const Structure& value,
                const UaNodeId& destDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Variant value of a UaStructureField to a UaVariant.
        // The returned Variant instance must be destroyed by the caller.
        UaVariant* convertIoStructureField2ua(const UaStructureField& field,
                const Variant& value, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array to an UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2ua(const Array& value,
                const UaNodeId& destDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array with elements of type Structure to an UaVariant.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2uaStructureArray(const Array& value,
                const UaNodeId& destDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array with elements of type Long to an UaVariant of type UInt16.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2uaUInt16Array(const Array& value, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array with elements of type Long to an UaVariant of type Int32.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2uaInt32Array(const Array& value, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array with elements of type ULong/LLong to an UaVariant of type UInt32.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2uaUInt32Array(const Array& value, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array with elements of type String to an UaVariant of type String.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2uaStringArray(const Array& value, OpcUa_Int16 indent) /* throws ConversionException */;
        // Converts an Array with elements of type LocalizedText to an UaVariant of type LocalizedText.
        // The returned UaVariant instance must be destroyed by the caller.
        UaVariant* convertIoArray2uaLocalizedTextArray(const Array& value, OpcUa_Int16 indent) /* throws ConversionException */;
    };

    ConverterUa2IO::ConverterUa2IO(ConverterCallback& callback, bool attachValues) {
        d = new ConverterUa2IOPrivate();
        d->log = LoggerFactory::getLogger("ConverterUa2IO");
        d->parent = this;
        d->callback = &callback;
        d->hasAttachedValues = attachValues;
    }

    ConverterUa2IO::~ConverterUa2IO() {
        if (d->hasAttachedValues) {
            delete d->callback;
        }
        delete d;
    }

    NodeId* ConverterUa2IO::convertUa2io(const UaNodeId & nodeId) const /* throws ConversionException */ {
        switch (nodeId.identifierType()) {
            case OpcUa_IdentifierType_Numeric:
                return new NodeId(nodeId.namespaceIndex(), nodeId.identifierNumeric());
            case OpcUa_IdentifierType_String:
                return new NodeId(nodeId.namespaceIndex(),
                        *new std::string(UaString(nodeId.identifierString()).toUtf8()),
                        true /* attachValues */);
            case OpcUa_IdentifierType_Guid:
            case OpcUa_IdentifierType_Opaque:
            default:
                std::ostringstream msg;
                msg << "Cannot convert UaNodeId of type " << nodeId.identifierType() << " to NodeId";
                throw ExceptionDef(ConversionException, msg.str());
        }
    }

    Variant* ConverterUa2IO::convertUa2io(const UaVariant& value,
            const UaNodeId& srcDataTypeId) /* throws ConversionException */ {
        Variant* ret = d->convertUa2io(value, srcDataTypeId, 0 /*indent*/); // ConversionException        
        if (d->log->isDebugEnabled() &&
                ret != NULL && (ret->getVariantType() == Variant::STRUCTURE
                || ret->getVariantType() == Variant::ARRAY)) {
            d->log->debug("> %s -> %s", value.toFullString().toUtf8(), ret->toString().c_str());
        }
        return ret;
    }

    UaNodeId * ConverterUa2IO::convertIo2ua(const NodeId& nodeId) const /* throws ConversionException */ {
        switch (nodeId.getNodeType()) {
            case NodeId::NUMERIC:
                return new UaNodeId(nodeId.getNumeric(), nodeId.getNamespaceIndex());
            case NodeId::STRING:
                return new UaNodeId(UaString(nodeId.getString().c_str()),
                        nodeId.getNamespaceIndex());
            default:
                std::ostringstream msg;
                msg << "Cannot convert NodeId of type " << nodeId.getNodeType() << " to UaNodeId";
                throw ExceptionDef(ConversionException, msg.str());
        }
    }

    UaVariant* ConverterUa2IO::convertIo2ua(const Variant& value,
            const UaNodeId& destDataTypeId) const /* throws ConversionException */ {
        UaVariant* ret = d->convertIo2ua(value, destDataTypeId, 0 /*indent*/); // ConversionException
        if (d->log->isDebugEnabled() && (value.getVariantType() == Variant::STRUCTURE
                || value.getVariantType() == Variant::ARRAY)) {
            d->log->debug("< %s -> %s", value.toString().c_str(), ret->toFullString().toUtf8());
        }
        return ret;
    }

    UaNodeId ConverterUa2IOPrivate::getBuildInType(const UaNodeId& typeId)
    /* throws ConversionException */ {
        if (0 == typeId.namespaceIndex()) {
            return typeId;
        }
        std::vector<UaNodeId>* superTypes = callback->getSuperTypes(typeId); // ConversionException
        if (superTypes != NULL) {
            ScopeGuard<std::vector<UaNodeId> > superTypesSG(superTypes);
            if (superTypes->size() > 0) {
                return superTypes->back();
            }
        }
        throw ExceptionDef(ConversionException,
                std::string("Cannot get base type of ").append(typeId.toXmlString().toUtf8()));
    }

    Variant* ConverterUa2IOPrivate::convertUa2io(const UaVariant& value, const UaNodeId& srcDataTypeId,
            OpcUa_Int16 indent) /* throws ConversionException */ {
        // OPC UA          - InternalInterface
        // boolean(8)      - bool(8)
        // sbyte(8)        - schar(8)
        // byte(8)         - char(8)
        // int16           - int(16)
        // uint16          - uint(16)
        // int32/enum      - long(32)
        // uint32          - ulong(32)
        // int64/dateTime  - llong(64)
        // uint64          - ullong(64)
        // float           - float
        // double          - double
        // string          - string
        // byteString      - byteString
        // localizedText   - localizedText
        // int32[]/enum[]  - long[]
        // uint32[]        - ulong[]]
        // localizedText[] - localizedText[]

        if (value.isArray()) {
            return convertUaArray2io(value, srcDataTypeId, indent); // ConversionException            
        } else {
            Variant* ret = NULL;
            UaNodeId buildInDataTypeId = getBuildInType(srcDataTypeId); // ConversionException                        
            switch (buildInDataTypeId.identifierNumeric()) {
                case OpcUaId_Structure:
                case OpcUaId_Union:
                {
                    UaExtensionObject eo;
                    value.toExtensionObject(eo);
                    ret = convertUaExtensionObject2io(eo, srcDataTypeId, indent); // ConversionException
                    break;
                }
                default:
                    ret = convertUaBuildInType2io(value, buildInDataTypeId, indent);
                    break;
            }
            if (ret == NULL) {
                throw ExceptionDef(ConversionException,
                        std::string("Cannot convert UaVariant of type ")
                        .append(srcDataTypeId.toXmlString().toUtf8())
                        .append(" to Variant"));
            }
            return ret;
        }
    }

    Variant * ConverterUa2IOPrivate::convertUaBuildInType2io(
            const UaVariant& value, const UaNodeId& srcDataTypeId, OpcUa_Int16 indent) {
        Variant* ret = NULL;
        if (0 == srcDataTypeId.namespaceIndex()) {
            switch (srcDataTypeId.identifierNumeric()) {
                case OpcUaType_Boolean: // 1
                    switch (value.type()) {
                        case OpcUaType_Boolean:
                        {
                            OpcUa_Boolean booleanValue;
                            if (OpcUa_IsGood(value.toBool(booleanValue))) {
                                Scalar* s = new Scalar();
                                s->setBool(static_cast<bool> (booleanValue));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type bool";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_SByte: // 2
                    switch (value.type()) {
                        case OpcUaType_SByte:
                        {
                            OpcUa_SByte sbyteValue;
                            if (OpcUa_IsGood(value.toSByte(sbyteValue))) {
                                Scalar* s = new Scalar();
                                s->setSChar(static_cast<signed char> (sbyteValue));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type schar";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_Byte: // 3
                    switch (value.type()) {
                        case OpcUaType_Byte:
                        {
                            OpcUa_Byte byteValue;
                            if (OpcUa_IsGood(value.toByte(byteValue))) {
                                Scalar* s = new Scalar();
                                s->setChar(static_cast<char> (byteValue));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type char";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_Int16: // 4
                    switch (value.type()) {
                        case OpcUaType_Int16:
                        {
                            OpcUa_Int16 int16Value;
                            if (OpcUa_IsGood(value.toInt16(int16Value))) {
                                Scalar* s = new Scalar();
                                s->setInt(static_cast<int> (int16Value));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type int";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_UInt16: // 5
                    switch (value.type()) {
                        case OpcUaType_UInt16:
                        {
                            OpcUa_UInt16 uint16Value;
                            if (OpcUa_IsGood(value.toUInt16(uint16Value))) {
                                Scalar* s = new Scalar();
                                s->setUInt(static_cast<unsigned int> (uint16Value));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type uint";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_Int32: // 6
                case OpcUaId_Enumeration: // 29   
                    switch (value.type()) {
                        case OpcUaType_UInt16:
                        {
                            OpcUa_UInt16 uint16Value;
                            if (OpcUa_IsGood(value.toUInt16(uint16Value))) {
                                Scalar* s = new Scalar();
                                s->setLong(static_cast<long> (uint16Value));
                                ret = s;
                            }
                            break;
                        }
                        case OpcUaType_Int32:
                        {
                            OpcUa_Int32 int32Value;
                            if (OpcUa_IsGood(value.toInt32(int32Value))) {
                                Scalar* s = new Scalar();
                                s->setLong(static_cast<long> (int32Value));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type long";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_UInt32: // 7
                    switch (value.type()) {
                        case OpcUaType_UInt32:
                        {
                            OpcUa_UInt32 uint32Value;
                            if (OpcUa_IsGood(value.toUInt32(uint32Value))) {
                                Scalar* s = new Scalar();
                                s->setULong(static_cast<unsigned long> (uint32Value));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type ulong";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_Int64: // 8
                    switch (value.type()) {
                        case OpcUaType_Int64:
                        {
                            OpcUa_Int64 int64Value;
                            if (OpcUa_IsGood(value.toInt64(int64Value))) {
                                Scalar* s = new Scalar();
                                s->setLLong(static_cast<long long> (int64Value));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type llong";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_UInt64: // 9
                    switch (value.type()) {
                        case OpcUaType_UInt64:
                        {
                            OpcUa_UInt64 uint64Value;
                            if (OpcUa_IsGood(value.toUInt64(uint64Value))) {
                                Scalar* s = new Scalar();
                                s->setULLong(static_cast<unsigned long long> (uint64Value));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type ullong";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_Float: // 10
                    switch (value.type()) {
                        case OpcUaType_Float:
                        {
                            OpcUa_Float floatValue;
                            if (OpcUa_IsGood(value.toFloat(floatValue))) {
                                Scalar* s = new Scalar();
                                s->setFloat(static_cast<float> (floatValue));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type float";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_Double: // 11
                case OpcUaId_Duration: // 290
                    switch (value.type()) {
                        case OpcUaType_Double:
                        {
                            OpcUa_Double doubleValue;
                            if (OpcUa_IsGood(value.toDouble(doubleValue))) {
                                Scalar* s = new Scalar();
                                s->setDouble(static_cast<double> (doubleValue));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type double";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_String: // 12
                    switch (value.type()) {
                        case OpcUaType_String:
                        {
                            Scalar* s = new Scalar();
                            if (value.toString().isNull()) {
                                s->setString(NULL, false /* attachValues */);
                            } else {
                                s->setString(new std::string(value.toString().toUtf8()), true /* attachValues */);
                            }
                            ret = s;
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type string";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_DateTime: // 13
                case OpcUaId_UtcTime: // 294
                    switch (value.type()) {
                        case OpcUaType_DateTime:
                        {
                            UaDateTime dateTimeValue;
                            if (OpcUa_IsGood(value.toDateTime(dateTimeValue))) {
                                Scalar* s = new Scalar();
                                s->setLLong(static_cast<long long> ((OpcUa_Int64) dateTimeValue));
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type llong";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_ByteString: // 15
                    switch (value.type()) {
                        case OpcUaType_ByteString:
                        {
                            Scalar* s = new Scalar();
                            UaByteString byteString;
                            value.toByteString(byteString);
                            if (byteString.data() == NULL) {
                                s->setByteString(NULL, -1 /* length */, false /* attachValues*/);
                            } else {
                                const OpcUa_Byte* bytes = byteString.data();
                                char* chars = new char[byteString.length()];
                                for (size_t i = 0; i < byteString.length(); i++) {
                                    chars[i] = bytes[i];
                                }
                                s->setByteString(chars, byteString.length(), true /* attachValues */);
                            }
                            ret = s;
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type byteString";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_NodeId:
                    switch (value.type()) {
                        case OpcUaType_NodeId:
                        {
                            UaNodeId nodeId;
                            value.toNodeId(nodeId);
                            switch (nodeId.identifierType()) {
                                case OpcUa_IdentifierType_Numeric:
                                    ret = new NodeId(nodeId.namespaceIndex(), nodeId.identifierNumeric());
                                    break;
                                case OpcUa_IdentifierType_String:
                                {
                                    std::string* id = new std::string(UaString(nodeId.identifierString()).toUtf8());
                                    ret = new NodeId(nodeId.namespaceIndex(), *id, true /*attachValues*/);
                                    break;
                                }
                                default:
                                    std::ostringstream msg;
                                    msg << "Cannot convert UaVariant of type " << value.type()
                                            << "/" << srcDataTypeId.toXmlString().toUtf8()
                                            << " to Scalar of type nodeId: Unsupported identifier type "
                                            << nodeId.identifierType();
                                    throw ExceptionDef(ConversionException, msg.str());
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type nodeId";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                case OpcUaType_LocalizedText: // 21
                    switch (value.type()) {
                        case OpcUaType_LocalizedText:
                        {
                            UaLocalizedText lt;
                            if (OpcUa_IsGood(value.toLocalizedText(lt))) {
                                Scalar* s = new Scalar();
                                std::string* ltLocale = NULL;
                                std::string* ltText = NULL;
                                if (!lt.isNull()) {
                                    const OpcUa_String* locale = lt.locale();
                                    if (locale != NULL) {
                                        ltLocale = new std::string(UaString(*locale).toUtf8());
                                    }
                                    const OpcUa_String* text = lt.text();
                                    if (text != NULL) {
                                        ltText = new std::string(UaString(*text).toUtf8());
                                    }
                                }
                                s->setLocalizedText(ltLocale, ltText, true /* attachValues */);
                                ret = s;
                            }
                            break;
                        }
                        default:
                        {
                            std::ostringstream msg;
                            msg << "Cannot convert UaVariant of type " << value.type()
                                    << "/" << srcDataTypeId.toXmlString().toUtf8()
                                    << " to Scalar of type localizedText";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                    }
                    break;
                default:
                {
                    std::ostringstream msg;
                    msg << "Cannot convert UaVariant of type " << value.type()
                            << "/" << srcDataTypeId.toXmlString().toUtf8() << " to Scalar";
                    throw ExceptionDef(ConversionException, msg.str());
                }
            }
        }
        return ret;
    }

    Variant * ConverterUa2IOPrivate::convertUaExtensionObject2io(
            const UaExtensionObject& value, const UaNodeId& srcDataTypeId, OpcUa_Int16 indent)
    /* throws ConversionException */ {
        NodeId* nodeId = parent->convertUa2io(srcDataTypeId); // ConversionException
        ScopeGuard<NodeId> nodeIdSG(nodeId);
        UaStructureDefinition sd = callback->getStructureDefinition(srcDataTypeId); // ConversionException
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("> %sstruct name=%s,dataTypeId=%s,isUnion=%d", ind,
                    sd.name().toUtf8(), sd.dataTypeId().toXmlString().toUtf8(), sd.isUnion());
        }
        std::map<std::string, const Variant*>* fieldData = new std::map<std::string, const Variant*>();
        MapScopeGuard<std::string, const Variant> fieldDataSG(fieldData);
        if (sd.isUnion()) {
            UaGenericUnionValue uv(value, sd);
            UaStructureField sf = uv.field();
            if (!sf.isNull()) {
                Variant* v = convertUaStructureField2io(sf, uv.value(), indent + 1); // ConversionException
                std::string fieldName(sf.name().toUtf8());
                (*fieldData)[fieldName] = v;
            }
        } else {
            UaGenericStructureValue sv(value, sd);
            for (int j = 0; j < sd.childrenCount(); j++) {
                UaStructureField sf = sd.child(j);
                std::string fieldName(sf.name().toUtf8());
                UaString uaFieldName(fieldName.c_str());
                if (!sf.isOptional() && !sv.isFieldSet(uaFieldName)) {
                    throw ExceptionDef(ConversionException,
                            std::string("Cannot convert UaExtensionObject of type ")
                            .append(srcDataTypeId.toXmlString().toUtf8())
                            .append(" due to missing value for mandatory field '")
                            .append(sf.name().toUtf8())
                            .append("'"));
                }
                if (sv.isFieldSet(uaFieldName)) {
                    OpcUa_StatusCode status;
                    UaVariant fieldValue = sv.value(uaFieldName, &status);
                    if (!OpcUa_IsGood(status)) {
                        throw ExceptionDef(ConversionException,
                                std::string("Cannot get value of field '")
                                .append(fieldName.c_str())
                                .append("' of structure with data type ")
                                .append(sd.dataTypeId().toXmlString().toUtf8()));
                    }
                    Variant* v = convertUaStructureField2io(sf, fieldValue, indent + 1); // ConversionException                                       
                    (*fieldData)[fieldName] = v;
                }
            }
        }
        return new Structure(*nodeIdSG.detach(), *fieldDataSG.detach(), true /*attachValues*/);
    }

    Variant* ConverterUa2IOPrivate::convertUaStructureField2io(const UaStructureField& field,
            const UaVariant& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("> %sfield name=%s,dataTypeId=%s,arrayType=%d", ind,
                    field.name().toUtf8(), field.typeId().toXmlString().toUtf8(), field.arrayType());
        }
        Variant* v = convertUa2io(value, field.typeId(), indent); // ConversionException                
        if (log->isTraceEnabled() && v->getVariantType() != Variant::ARRAY) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("> %s-> %s", ind, v->toString().c_str());
        }
        return v;
    }

    IODataProviderNamespace::Array* ConverterUa2IOPrivate::convertUaArray2io(
            const UaVariant& value, const UaNodeId& srcDataTypeId, OpcUa_Int16 indent)
    /* throws ConversionException */ {
        UaNodeId buildInDataTypeId = getBuildInType(srcDataTypeId); // ConversionException
        int arrayType = getIoArrayType(buildInDataTypeId); // ConversionException
        std::vector<const Variant*>* elements = new std::vector<const Variant*>();
        VectorScopeGuard<const Variant> elementsSG(elements);
        for (OpcUa_UInt32 i = 0; i < value.arraySize(); i++) {
            UaVariant uav(value[i]);
            if (uav.isArray()) {
                throw ExceptionDef(ConversionException,
                        std::string("Cannot convert Array of type ")
                        .append(srcDataTypeId.toXmlString().toUtf8())
                        .append(" to Variant due to unsupported nested Variant arrays"));
            }
            Variant* v = convertUa2io(uav, srcDataTypeId, indent + 1); // ConversionException 
            elements->push_back(v);
        }
        return new Array(arrayType, elementsSG.detach(), true /*attachValues*/);
    }

    int ConverterUa2IOPrivate::getIoArrayType(const UaNodeId dataTypeId) /* throws ConversionException */ {
        if (0 == dataTypeId.namespaceIndex()) {
            switch (dataTypeId.identifierNumeric()) {
                case OpcUaType_Int32:
                case OpcUaId_Enumeration:
                    return Scalar::LONG;
                case OpcUaType_UInt32:
                    return Scalar::ULONG;
                case OpcUaType_String:
                    return Scalar::STRING;
                case OpcUaId_LocalizedText:
                    return Scalar::LOCALIZED_TEXT;
                case OpcUaId_Structure:
                case OpcUaId_Union:
                    return Variant::STRUCTURE;
            }
        }
        throw ExceptionDef(ConversionException,
                std::string("Cannot get Array type for ")
                .append(dataTypeId.toXmlString().toUtf8()));
    }

    UaVariant * ConverterUa2IOPrivate::convertIo2ua(const Variant& value,
            const UaNodeId& destDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */ {
        // InternalInterface - OPC UA
        // bool(8)           - boolean(8)
        // schar(8)          - sbyte(8)
        // char(8)           - byte(8)
        // int(16)           - int16
        // uint(16)          - uint16
        // long(32)          - int32/uint16/enum
        // ulong(32)         - uint32
        // llong(64)         - int64/uint32/dateTime/utcTime
        // ullong(64)        - uint64
        // float             - float
        // double            - double/duration
        // string            - string/localizedText
        // byteString        - byteString
        // localizedText     - localizedText
        // long[]            - int32[]/uint16[]/enum[]
        // ulong[]           - uint32[]
        // localizedText[]   - localizedText[]

        UaVariant* ret;
        switch (value.getVariantType()) {
            case Variant::ARRAY:
                ret = convertIoArray2ua(static_cast<const Array&> (value), destDataTypeId,
                        indent); // ConversionException
                break;
            case Variant::STRUCTURE:
                ret = convertIoStructure2ua(static_cast<const Structure&> (value), destDataTypeId,
                        indent); // ConversionException
                break;
            case Variant::SCALAR:
            {
                UaNodeId buildInDataTypeId = getBuildInType(destDataTypeId); // ConversionException                
                ret = convertIoScalar2ua(static_cast<const Scalar&> (value), buildInDataTypeId,
                        indent); // ConversionException
                break;
            }
            case Variant::NODE_ID:
                //TODO
            default:
                std::ostringstream msg;
                msg << "Cannot convert Variant of type " << value.getVariantType()
                        << " to UaVariant of type " << destDataTypeId.toXmlString().toUtf8();
                throw ExceptionDef(ConversionException, msg.str());
        }
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoScalar2ua(
            const Scalar& value, const UaNodeId& destDataTypeId, OpcUa_Int16 indent) const
    /* throws ConversionException */ {
        UaVariant* ret = NULL;
        switch (value.getScalarType()) {
            case Scalar::BOOL:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Boolean:
                        ret = new UaVariant();
                        ret->setBool(value.getBool());
                        break;
                }
                break;
            case Scalar::SCHAR:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_SByte:
                        ret = new UaVariant();
                        ret->setSByte(value.getSChar());
                        break;
                }
                break;
            case Scalar::CHAR:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Byte:
                        ret = new UaVariant();
                        ret->setByte(value.getChar());
                        break;
                }
                break;
            case Scalar::INT:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Int16:
                        ret = new UaVariant();
                        ret->setInt16(value.getInt());
                        break;
                }
                break;
            case Scalar::UINT:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_UInt16:
                        ret = new UaVariant();
                        ret->setUInt16(value.getUInt());
                        break;
                }
                break;
            case Scalar::LONG:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Int32:
                    case OpcUaId_Enumeration:
                        ret = new UaVariant();
                        ret->setInt32(value.getLong());
                        break;
                    case OpcUaType_UInt16:
                    {
                        long v = value.getLong();
                        if (v >= 0 && v < 2^15) {
                            ret = new UaVariant();
                            ret->setUInt16(v);
                        } else {
                            std::ostringstream msg;
                            msg << "Invalid value " << v << " for destination type uint16";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                        break;
                    }
                }
                break;
            case Scalar::ULONG:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_UInt32:
                        ret = new UaVariant();
                        ret->setUInt32(value.getULong());
                        break;
                }
                break;
            case Scalar::LLONG:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Int64:
                        ret = new UaVariant();
                        ret->setInt64(value.getLLong());
                        break;
                    case OpcUaType_UInt32:
                    {
                        long long v = value.getLLong();
                        if (v >= 0 && v < 2^31) {
                            ret = new UaVariant();
                            ret->setUInt32(v);
                        } else {
                            std::ostringstream msg;
                            msg << "Invalid value " << v << " for destination type OpcUa_UInt32";
                            throw ExceptionDef(ConversionException, msg.str());
                        }
                        break;
                    }
                    case OpcUaType_DateTime:
                    case OpcUaId_UtcTime:
                        ret = new UaVariant();
                        ret->setDateTime(UaDateTime(static_cast<OpcUa_Int64> (value.getLLong())));
                        break;
                }
                break;
            case Scalar::ULLONG:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_UInt64:
                        ret = new UaVariant();
                        ret->setUInt64(value.getULLong());
                        break;
                }
                break;
            case Scalar::FLOAT:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Float:
                        ret = new UaVariant();
                        ret->setFloat(value.getFloat());
                        break;
                }
                break;
            case Scalar::DOUBLE:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_Double:
                    case OpcUaId_Duration:
                        ret = new UaVariant();
                        ret->setDouble(value.getDouble());
                        break;
                }
                break;
            case Scalar::STRING:
            {
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_String:
                        ret = new UaVariant();
                        if (value.getString() == NULL) {
                            UaString str;
                            ret->setString(str);
                        } else {
                            ret->setString(UaString(value.getString()->c_str()));
                        }
                        break;
                    case OpcUaType_LocalizedText:
                        ret = new UaVariant();
                        if (value.getString() == NULL) {
                            UaLocalizedText lt;
                            ret->setLocalizedText(lt);
                        } else {
                            UaLocalizedText lt(UaString("en"), UaString(value.getString()->c_str()));
                            ret->setLocalizedText(lt);
                        }
                        break;
                }
                break;
            }
            case Scalar::BYTE_STRING:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_ByteString:
                    {
                        const char* bs = value.getByteString();
                        ret = new UaVariant();
                        if (bs == NULL) {
                            UaByteString byteString;
                            ret->setByteString(byteString, true /*detach*/);
                        } else {
                            long length = value.getByteStringLength();
                            OpcUa_Byte bytes[length];
                            for (size_t i = 0; i < length; i++) {
                                bytes[i] = bs[i];
                            }
                            UaByteString byteString(length, bytes);
                            ret->setByteString(byteString, true /*detach*/);
                        }
                        break;
                    }
                }
                break;
            case Scalar::LOCALIZED_TEXT:
                switch (destDataTypeId.identifierNumeric()) {
                    case OpcUaType_LocalizedText:
                    {
                        ret = new UaVariant();
                        UaLocalizedText lt;
                        const std::string* locale = value.getLocalizedTextLocale();
                        if (locale != NULL) {
                            UaString uaLocale(locale->c_str());
                            lt.setLocale(uaLocale);
                        }
                        const std::string* text = value.getLocalizedTextText();
                        if (text != NULL) {
                            UaString uaText(text->c_str());
                            lt.setText(uaText);
                        }
                        ret->setLocalizedText(lt);
                        break;
                    }
                }
                break;
        }
        if (ret == NULL) {
            std::ostringstream msg;
            msg << "Cannot convert Scalar of type " << value.getScalarType()
                    << " to UaVariant of type " << destDataTypeId.toXmlString().toUtf8();
            throw ExceptionDef(ConversionException, msg.str());
        }
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoStructure2ua(
            const Structure& value, const UaNodeId& destDataTypeId, OpcUa_Int16 indent) /* throws ConversionException */ {
        UaStructureDefinition sd = callback->getStructureDefinition(destDataTypeId); // ConversionException
        const std::map<std::string, const Variant*>& fieldData = value.getFieldData();
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("< %sstruct name=%s,dataTypeId=%s,isUnion=%d", ind,
                    sd.name().toUtf8(), sd.dataTypeId().toXmlString().toUtf8(), sd.isUnion());
        }
        UaExtensionObject eo;
        if (sd.isUnion()) {
            UaGenericUnionValue uv(sd);
            if (fieldData.size() > 1) {
                std::ostringstream msg;
                msg << "Cannot convert Structure of type " << destDataTypeId.toXmlString().toUtf8()
                        << " due to too many values for a union type: " << fieldData.size();
                throw ExceptionDef(ConversionException, msg.str());
            }
            if (fieldData.size() == 1) {
                std::map<std::string, const Variant*>::const_iterator fieldValueIter
                        = fieldData.begin();
                UaString fieldName(fieldValueIter->first.c_str());
                bool found = false;
                // for each union field
                for (int j = 0; j < sd.childrenCount(); j++) {
                    UaStructureField sf = sd.child(j);
                    // if union field for value
                    if (sf.name() == fieldName) {
                        // convert value from Variant to UaVariant
                        UaVariant* fieldValue = convertIoStructureField2ua(sf,
                                *fieldValueIter->second, indent + 1); // ConversionException
                        // set value to union field
                        uv.setValue(sf.name(), *fieldValue);
                        delete fieldValue;

                        found = true;
                        break;
                    }
                }
                if (!found) {
                    throw ExceptionDef(ConversionException,
                            std::string("Cannot convert Structure of type ")
                            .append(destDataTypeId.toXmlString().toUtf8())
                            .append(" due to missing union field ")
                            .append(fieldName.toUtf8()));
                }
            }
            uv.toExtensionObject(eo);
        } else {
            UaGenericStructureValue sv(sd);
            // for each field
            for (int j = 0; j < sd.childrenCount(); j++) {
                UaStructureField sf = sd.child(j);
                std::string fieldName(sf.name().toUtf8());
                std::map<std::string, const Variant*>::const_iterator fieldValueIter
                        = fieldData.find(fieldName);
                if (!sf.isOptional() && fieldValueIter == fieldData.end()) {
                    throw ExceptionDef(ConversionException,
                            std::string("Cannot convert Structure of type ")
                            .append(destDataTypeId.toXmlString().toUtf8())
                            .append(" due to missing value for mandatory field ")
                            .append(sf.name().toUtf8()));
                }
                if (fieldValueIter != fieldData.end()) {
                    // convert value from Variant to UaVariant
                    UaVariant* fieldValue = convertIoStructureField2ua(sf,
                            *fieldValueIter->second, indent + 1); // ConversionException
                    // set value to field
                    sv.setField(sf.name(), *fieldValue);
                    delete fieldValue;
                }
            }
            sv.toExtensionObject(eo);
        }
        UaVariant* ret = new UaVariant();
        ret->setExtensionObject(eo, OpcUa_False /*detach*/);
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoStructureField2ua(const UaStructureField& field,
            const Variant& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("< %sfield name=%s,dataTypeId=%s,isOptional=%d,arrayType=%d", ind,
                    field.name().toUtf8(), field.typeId().toXmlString().toUtf8(), field.isOptional(),
                    field.arrayType());
        }
        UaVariant* fieldValue = convertIo2ua(value, field.typeId(), indent); // ConversionException
        if (log->isTraceEnabled()) {
            char ind[indent + 1];
            memset(ind, ' ', indent);
            ind[indent] = 0;
            log->trace("< %s-> type=%d,value=%s", ind, fieldValue->type(),
                    fieldValue->toFullString().toUtf8());
        }
        return fieldValue;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2ua(
            const Array& value, const UaNodeId& destDataTypeId, OpcUa_Int16 indent)
    /* throws ConversionException */ {

        // InternalInterface - OPC UA        
        // long[]            - int32[]/uint16[]/enum[]
        // ulong[]           - uint32[]
        // llong[]           - uint32[]
        // string[]          - string[]/localizedText[]
        // localizedText[]   - localizedText[]

        if (value.getElements() == NULL) {
            throw ExceptionDef(ConversionException,
                    std::string("A null array cannot be converted to UaVariant"));
        }
        UaVariant* ret = NULL;
        switch (value.getArrayType()) {
            case Variant::STRUCTURE:
                ret = convertIoArray2uaStructureArray(value, destDataTypeId, indent);
                break;
            default:
                UaNodeId buildInDataTypeId = getBuildInType(destDataTypeId); // ConversionException
                switch (value.getArrayType()) {
                    case Scalar::LONG:
                    {
                        switch (buildInDataTypeId.identifierNumeric()) {
                            case OpcUaType_Int32:
                            case OpcUaId_Enumeration:
                                ret = convertIoArray2uaInt32Array(value, indent);
                                break;
                            case OpcUaType_UInt16:
                                ret = convertIoArray2uaUInt16Array(value, indent);
                                break;
                        }
                        break;
                    }
                    case Scalar::ULONG:
                    {
                        switch (buildInDataTypeId.identifierNumeric()) {
                            case OpcUaType_UInt32:
                                ret = convertIoArray2uaUInt32Array(value, indent);
                                break;
                        }
                        break;
                    }
                    case Scalar::LLONG:
                    {
                        switch (buildInDataTypeId.identifierNumeric()) {
                            case OpcUaType_Int64:
                                //TODO
                                break;
                            case OpcUaType_UInt32:
                                ret = convertIoArray2uaUInt32Array(value, indent);
                                break;
                            case OpcUaType_DateTime:
                            case OpcUaId_UtcTime:
                                //TODO
                                break;
                        }
                        break;
                    }
                    case Scalar::STRING:
                    {
                        switch (buildInDataTypeId.identifierNumeric()) {
                            case OpcUaType_String:
                                ret = convertIoArray2uaStringArray(value, indent);
                                break;
                            case OpcUaType_LocalizedText:
                                ret = convertIoArray2uaLocalizedTextArray(value, indent);
                                break;
                        }
                        break;
                    }
                    case Scalar::LOCALIZED_TEXT:
                    {
                        switch (buildInDataTypeId.identifierNumeric()) {
                            case OpcUaType_LocalizedText:
                                ret = convertIoArray2uaLocalizedTextArray(value, indent);
                                break;
                        }
                        break;
                    }
                }
                break;
        }
        if (ret == NULL) {
            std::ostringstream msg;
            msg << "Cannot convert Array of type " << value.getArrayType()
                    << " to UaVariant of type " << destDataTypeId.toXmlString().toUtf8();
            throw ExceptionDef(ConversionException, msg.str());
        }
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2uaStructureArray(
            const Array& value, const UaNodeId& destDataTypeId, OpcUa_Int16 indent)
    /* throws ConversionException */ {
        const std::vector<const Variant*>* elements = value.getElements();
        UaExtensionObjectArray array;
        array.create(elements->size());
        // for each array element        
        for (int i = 0; i < elements->size(); i++) {
            const Structure& elem = *static_cast<const Structure*> ((*elements)[i]);
            UaVariant* v = convertIoStructure2ua(elem, destDataTypeId,
                    indent + 1); // ConversionException
            UaExtensionObject eo;
            v->toExtensionObject(eo);
            eo.copyTo(&array[i]);
            delete v;
        }
        UaVariant* ret = new UaVariant();
        ret->setExtensionObjectArray(array, OpcUa_True /*detach*/);
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2uaUInt16Array(
            const Array& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        const std::vector<const Variant*>* elements = value.getElements();
        UaUInt16Array array;
        array.create(elements->size());
        // for each array element
        for (int i = 0; i < elements->size(); i++) {
            const Scalar& elem = *static_cast<const Scalar*> ((*elements)[i]);
            switch (value.getArrayType()) {
                case Scalar::LONG:
                    array[i] = static_cast<OpcUa_UInt16> (elem.getLong());
                    break;
            }
        }
        UaVariant* ret = new UaVariant();
        ret->setUInt16Array(array, OpcUa_True /*detach*/);
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2uaInt32Array(
            const Array& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        const std::vector<const Variant*>* elements = value.getElements();
        UaInt32Array array;
        array.create(elements->size());
        // for each array element
        for (int i = 0; i < elements->size(); i++) {
            const Scalar& elem = *static_cast<const Scalar*> ((*elements)[i]);
            switch (value.getArrayType()) {
                case Scalar::LONG:
                    array[i] = static_cast<OpcUa_Int32> (elem.getLong());
                    break;
            }
        }
        UaVariant* ret = new UaVariant();
        ret->setInt32Array(array, OpcUa_True /*detach*/);
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2uaUInt32Array(
            const Array& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        const std::vector<const Variant*>* elements = value.getElements();
        UaUInt32Array array;
        array.create(elements->size());
        // for each array element
        for (int i = 0; i < elements->size(); i++) {
            const Scalar& elem = *static_cast<const Scalar*> ((*elements)[i]);
            switch (value.getArrayType()) {
                case Scalar::ULONG:
                    array[i] = static_cast<OpcUa_UInt32> (elem.getULong());
                    break;
                case Scalar::LLONG:
                    array[i] = static_cast<OpcUa_UInt32> (elem.getLLong());
                    break;
            }
        }
        UaVariant* ret = new UaVariant();
        ret->setUInt32Array(array, OpcUa_True /*detach*/);
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2uaStringArray(
            const Array& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        const std::vector<const Variant*>* elements = value.getElements();
        UaStringArray array;
        array.create(elements->size());
        // for each array element
        for (int i = 0; i < elements->size(); i++) {
            const Scalar& elem = *static_cast<const Scalar*> ((*elements)[i]);
            switch (value.getArrayType()) {
                case Scalar::STRING:
                    UaString str(elem.getString()->c_str());
                    str.copyTo(&array[i]);
                    break;
            }
        }
        UaVariant* ret = new UaVariant();
        ret->setStringArray(array, OpcUa_True /*detach*/);
        return ret;
    }

    UaVariant * ConverterUa2IOPrivate::convertIoArray2uaLocalizedTextArray(
            const Array& value, OpcUa_Int16 indent) /* throws ConversionException */ {
        const std::vector<const Variant*>* elements = value.getElements();
        UaLocalizedTextArray array;
        array.create(elements->size());
        // for each array element
        for (int i = 0; i < elements->size(); i++) {
            const Scalar& elem = *static_cast<const Scalar*> ((*elements)[i]);
            switch (value.getArrayType()) {
                case Scalar::STRING:
                {
                    UaLocalizedText lt;
                    lt.setLocale(UaString("en"));
                    lt.setText(UaString(elem.getString()->c_str()));
                    lt.copyTo(&array[i]);
                    break;
                }
                case Scalar::LOCALIZED_TEXT:
                {
                    UaLocalizedText lt;
                    const std::string* locale = elem.getLocalizedTextLocale();
                    if (locale != NULL) {
                        UaString uaLocale(locale->c_str());
                        lt.setLocale(uaLocale);
                    }
                    const std::string* text = elem.getLocalizedTextText();
                    if (text != NULL) {
                        UaString uaText(text->c_str());
                        lt.setText(uaText);
                    }
                    lt.copyTo(&array[i]);
                    break;
                }
            }
        }
        UaVariant* ret = new UaVariant();
        ret->setLocalizedTextArray(array, OpcUa_True /*detach*/);
        return ret;
    }

} // namespace SASModelProviderNamespace
