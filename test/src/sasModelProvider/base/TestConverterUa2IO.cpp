#include <sasModelProvider/base/CodeNodeManagerBase.h> 
// UaMutexLocker loaded via iomanageruanode.h in IODataManager.h overloads operator "new"
#include "CppUTest/TestHarness.h" 
#include "../../Env.h"
#include <common/Exception.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include "ioDataProvider/Array.h"
#include "ioDataProvider/Structure.h"
#include <ioDataProvider/MethodData.h>
#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/NodeProperties.h>
#include <ioDataProvider/SubscriberCallback.h>
#include <ioDataProvider/Variant.h>
#include <sasModelProvider/base/ConverterUa2IO.h>
#include <sasModelProvider/base/ConversionException.h>
#include <opcua_builtintypes.h> // OpcUaType_Int16
#include <uaextensionobject.h> // UaExtensionObject
#include <uanodeid.h> // UaNodeId
#include <uastring.h> // UaString
#include <map>
#include <stddef.h> // NULL
#include <string>
#include <vector>

using namespace CommonNamespace;
using namespace SASModelProviderNamespace;

namespace TestNamespace {

    TEST_GROUP(SasModelProviderBase_ConverterUa2IO) {
        ConsoleLoggerFactory clf;
        LoggerFactory* lf;

        void setup() {
            lf = new LoggerFactory(clf);
        }

        void teardown() {
            delete lf;
        }

        class ConverterCallback : public ConverterUa2IO::ConverterCallback {
        public:

            virtual void addStructureDefinition(UaStructureDefinition& structureDefinition) {
                std::string dataTypeId(structureDefinition.dataTypeId().toXmlString().toUtf8());
                structureDefinitions[dataTypeId] = &structureDefinition;
            }

            virtual void addDataTypeParents(UaNodeId& dataTypeId, std::vector<UaNodeId>& parents) {
                std::string dataTypeIdStr(dataTypeId.toXmlString().toUtf8());
                dataTypeParents[dataTypeIdStr] = &parents;
            }

            // interface Converter::ConverterCallback

            virtual UaStructureDefinition getStructureDefinition(const UaNodeId& dataTypeId) {
                return *structureDefinitions.at(std::string(dataTypeId.toXmlString().toUtf8()));
            }

            virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& dataTypeId) {
                std::vector<UaNodeId>* parents =
                        dataTypeParents.at(std::string(dataTypeId.toXmlString().toUtf8()));
                // return a copy
                std::vector<UaNodeId>* parentsCopy = new std::vector<UaNodeId>();
                for (std::vector<UaNodeId>::const_iterator it = parents->begin();
                        it != parents->end(); it++) {
                    parentsCopy->push_back(*it);
                }
                return parentsCopy;
            }
        private:
            std::map<std::string, UaStructureDefinition*> structureDefinitions;
            std::map<std::string, std::vector<UaNodeId>*> dataTypeParents;
        };
    };

    TEST(SasModelProviderBase_ConverterUa2IO, UaNodeId) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        UaNodeId nid1(3, 2 /*nsIndex*/);
        IODataProviderNamespace::NodeId* nodeId = conv.convertUa2io(nid1);
        CHECK_EQUAL(3, nodeId->getNumeric());
        CHECK_EQUAL(2, nodeId->getNamespaceIndex());
        delete nodeId;

        UaNodeId nid2(UaString("a"), 2 /*nsIndex*/);
        nodeId = conv.convertUa2io(nid2);
        STRCMP_EQUAL("a", nodeId->getString().c_str());
        CHECK_EQUAL(2, nodeId->getNamespaceIndex());
        delete nodeId;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, NodeId) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        IODataProviderNamespace::NodeId nid1(2 /*nsIndex*/, 3);
        UaNodeId* nodeId = conv.convertIo2ua(nid1);
        CHECK_EQUAL(3, nodeId->identifierNumeric());
        CHECK_EQUAL(2, nodeId->namespaceIndex());
        delete nodeId;

        std::string id("a");
        IODataProviderNamespace::NodeId nid2(2 /*nsIndex*/, id);
        nodeId = conv.convertIo2ua(nid2);
        STRCMP_EQUAL("a", UaString(nodeId->identifierString()).toUtf8());
        CHECK_EQUAL(2, nodeId->namespaceIndex());
        delete nodeId;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, UaVariantScalarValue) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // Server                 - InternalInterface
        // boolean(8)             - bool(8)
        // sbyte(8)               - schar(8)
        // byte(8)                - char(8)
        // int16                  - int(16)
        // uint16                 - uint(16)
        // int32/enum             - long(32)
        // uint32                 - ulong(32)
        // int64/dateTime/utcTime - llong(64)
        // uint64                 - ullong(64)
        // float                  - float
        // double/duration        - double
        // string                 - string
        // byteString             - byteString
        // localizedText          - localizedText

        // boolean
        UaVariant v;
        v.setBool(true);
        IODataProviderNamespace::Scalar* value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Boolean)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::BOOL, value->getScalarType());
        CHECK_TRUE(value->getBool());
        delete value;

        // sbyte
        v.setSByte(1);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_SByte)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::SCHAR, value->getScalarType());
        CHECK_EQUAL(1, value->getSChar());
        delete value;

        // byte
        v.setByte(2);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Byte)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::CHAR, value->getScalarType());
        CHECK_EQUAL(2, value->getChar());
        delete value;

        // int16
        v.setInt16(3);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Int16)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::INT, value->getScalarType());
        CHECK_EQUAL(3, value->getInt());
        delete value;

        // uint16
        v.setUInt16(4);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_UInt16)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::UINT, value->getScalarType());
        CHECK_EQUAL(4, value->getUInt());
        delete value;

        // int32
        v.setInt32(5);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Int32)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LONG, value->getScalarType());
        CHECK_EQUAL(5, value->getLong());
        delete value;

        // enum
        std::vector<UaNodeId> enumParents;
        enumParents.push_back(UaNodeId(OpcUaId_Enumeration));
        UaNodeId enumDataTypeId(34, 2 /* nsIndex */);
        callback.addDataTypeParents(enumDataTypeId, enumParents);

        value = static_cast<IODataProviderNamespace::Scalar*> (conv.convertUa2io(v, enumDataTypeId));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LONG, value->getScalarType());
        CHECK_EQUAL(5, value->getLong());
        delete value;

        // uint32
        v.setUInt32(6);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_UInt32)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::ULONG, value->getScalarType());
        CHECK_EQUAL(6, value->getULong());
        delete value;

        // int64
        v.setInt64(5);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Int64)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LLONG, value->getScalarType());
        LONGS_EQUAL(5, value->getLLong());
        delete value;

        // dateTime
        UaString sTime("2012-05-30T09:30:10Z");
        UaDateTime dt(UaDateTime::fromString(sTime));
        v.setDateTime(dt);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_DateTime)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LLONG, value->getScalarType());
        // 129828438100000000 / 10 / 1000 / 1000 / 86400 / ~365.25 
        // -> 411... years since 1601-01-01
        LONGS_EQUAL(129828438100000000, value->getLLong());
        delete value;

        // utcTime
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaId_UtcTime)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LLONG, value->getScalarType());
        LONGS_EQUAL(129828438100000000, value->getLLong());
        delete value;

        // uint64
        v.setUInt64(6);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_UInt64)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::ULLONG, value->getScalarType());
        LONGS_EQUAL(6, value->getULLong());
        delete value;

        // float
        v.setFloat(7.7);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Float)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::FLOAT, value->getScalarType());
        DOUBLES_EQUAL(7.7, value->getFloat(), 0.1);
        delete value;

        // double
        v.setDouble(8.8);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_Double)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::DOUBLE, value->getScalarType());
        DOUBLES_EQUAL(8.8, value->getDouble(), 0.1);
        delete value;

        // duration
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaId_Duration)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::DOUBLE, value->getScalarType());
        DOUBLES_EQUAL(8.8, value->getDouble(), 0.1);
        delete value;

        // string
        v.setString(UaString("a"));
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_String)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::STRING, value->getScalarType());
        STRCMP_EQUAL("a", value->getString()->c_str());
        delete value;

        UaString str;
        v.setString(str);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_String)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::STRING, value->getScalarType());
        CHECK_TRUE(value->getString() == NULL);
        delete value;

        // byteString
        OpcUa_Byte bytes[2];
        bytes[0] = 34;
        bytes[1] = 255;
        UaByteString byteString(2 /* length */, bytes);
        v.setByteString(byteString, true /* detach*/);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_ByteString)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::BYTE_STRING, value->getScalarType());
        CHECK_EQUAL(2, value->getByteStringLength());
        const char* bs = value->getByteString();
        CHECK_EQUAL(34, bs[0] & 0xFF);
        CHECK_EQUAL(255, bs[1] & 0xFF);
        delete value;

        UaByteString byteStr;
        v.setByteString(byteStr, true /*attachValues*/);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_ByteString)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::BYTE_STRING, value->getScalarType());
        CHECK_EQUAL(-1, value->getByteStringLength());
        CHECK_TRUE(value->getByteString() == NULL);
        delete value;

        // localizedText
        UaLocalizedText localizedTextValue(UaString("a"), UaString("b"));
        v.setLocalizedText(localizedTextValue);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_LocalizedText)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LOCALIZED_TEXT, value->getScalarType());
        STRCMP_EQUAL("a", value->getLocalizedTextLocale()->c_str());
        STRCMP_EQUAL("b", value->getLocalizedTextText()->c_str());
        delete value;

        UaLocalizedText localizedTextValue2;
        v.setLocalizedText(localizedTextValue2);
        value = static_cast<IODataProviderNamespace::Scalar*> (
                conv.convertUa2io(v, UaNodeId(OpcUaType_LocalizedText)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LOCALIZED_TEXT, value->getScalarType());
        CHECK_TRUE(value->getLocalizedTextLocale() == NULL);
        CHECK_TRUE(value->getLocalizedTextText() == NULL);
        delete value;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, UaVariantScalarValueError) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // boolean
        UaVariant v;
        v.setBool(true);
        try {
            static_cast<IODataProviderNamespace::Scalar*> (
                    conv.convertUa2io(v, UaNodeId(OpcUaId_Enumeration)));
            FAIL("");
        } catch (ConversionException& e) {
            STRCMP_CONTAINS("type 1/i=29 to Scalar of type long", e.getMessage().c_str());
        } catch (Exception& e) {
            FAIL("");
        }
    }

    TEST(SasModelProviderBase_ConverterUa2IO, UaVariantArrayValue) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // Server                 - InternalInterface
        // int32[]/enum[]         - long[]
        // uint32[]               - ulong[]
        // string[]               - string[]
        // localizedText[]        - localizedText[]

        // int32[] -> long[]
        UaInt32Array valueInt32Array;
        valueInt32Array.create(2);
        valueInt32Array[0] = 3;
        valueInt32Array[1] = 4;
        UaVariant valueInt32;
        valueInt32.setInt32Array(valueInt32Array);
        IODataProviderNamespace::Array* arrayInt32 =
                static_cast<IODataProviderNamespace::Array*> (
                conv.convertUa2io(valueInt32, UaNodeId(OpcUaType_Int32)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LONG, arrayInt32->getArrayType());
        const std::vector<const IODataProviderNamespace::Variant*>* elementsInt32 = arrayInt32->getElements();
        CHECK_EQUAL(2, elementsInt32->size());
        LONGS_EQUAL(3, static_cast<const IODataProviderNamespace::Scalar*> (elementsInt32->at(0))->getLong());
        LONGS_EQUAL(4, static_cast<const IODataProviderNamespace::Scalar*> (elementsInt32->at(1))->getLong());
        delete arrayInt32;

        // enum[] -> long[]
        std::vector<UaNodeId> enumParents;
        enumParents.push_back(UaNodeId(OpcUaId_Enumeration));
        UaNodeId enumDataTypeId(34, 2 /* nsIndex */);
        callback.addDataTypeParents(enumDataTypeId, enumParents);

        arrayInt32 = static_cast<IODataProviderNamespace::Array*> (
                conv.convertUa2io(valueInt32, enumDataTypeId));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LONG, arrayInt32->getArrayType());
        elementsInt32 = arrayInt32->getElements();
        CHECK_EQUAL(2, elementsInt32->size());
        LONGS_EQUAL(3, static_cast<const IODataProviderNamespace::Scalar*> (elementsInt32->at(0))->getLong());
        LONGS_EQUAL(4, static_cast<const IODataProviderNamespace::Scalar*> (elementsInt32->at(1))->getLong());
        delete arrayInt32;

        // uint32[] -> ulong[]
        UaUInt32Array valueUInt32Array;
        valueUInt32Array.create(2);
        valueUInt32Array[0] = 3;
        valueUInt32Array[1] = 4;
        UaVariant valueUInt32;
        valueUInt32.setUInt32Array(valueUInt32Array);
        IODataProviderNamespace::Array* arrayUInt32 =
                static_cast<IODataProviderNamespace::Array*> (
                conv.convertUa2io(valueUInt32, UaNodeId(OpcUaType_UInt32)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::ULONG, arrayUInt32->getArrayType());
        const std::vector<const IODataProviderNamespace::Variant*>* elementsUInt32 = arrayUInt32->getElements();
        CHECK_EQUAL(2, elementsUInt32->size());
        LONGS_EQUAL(3, static_cast<const IODataProviderNamespace::Scalar*> (elementsUInt32->at(0))->getLong());
        LONGS_EQUAL(4, static_cast<const IODataProviderNamespace::Scalar*> (elementsUInt32->at(1))->getLong());
        delete arrayUInt32;
                
        // string[] -> string[]
        UaStringArray valueStringArray;
        valueStringArray.create(2);
        UaString("a").copyTo(&valueStringArray[0]);
        UaString("bc").copyTo(&valueStringArray[1]);
        UaVariant valueString;
        valueString.setStringArray(valueStringArray);
        IODataProviderNamespace::Array* arrayString =
                static_cast<IODataProviderNamespace::Array*> (
                conv.convertUa2io(valueString, UaNodeId(OpcUaType_String)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::STRING, arrayString->getArrayType());
        const std::vector<const IODataProviderNamespace::Variant*>* elementsString = arrayString->getElements();
        CHECK_EQUAL(2, elementsString->size());
        STRCMP_EQUAL("a", static_cast<const IODataProviderNamespace::Scalar*> (elementsString->at(0))->getString()->c_str());
        STRCMP_EQUAL("bc", static_cast<const IODataProviderNamespace::Scalar*> (elementsString->at(1))->getString()->c_str());
        delete arrayString;

        // localizedText[] -> localizedText[]
        UaLocalizedTextArray valueLocalizedTextArray;
        valueLocalizedTextArray.create(2);
        UaLocalizedText lt;
        lt.setLocalizedText(UaString("en"), UaString("a"));
        lt.copyTo(&valueLocalizedTextArray[0]);
        lt.setLocalizedText(UaString("de"), UaString("b"));
        lt.copyTo(&valueLocalizedTextArray[1]);
        UaVariant valueLocalizedText;
        valueLocalizedText.setLocalizedTextArray(valueLocalizedTextArray);
        IODataProviderNamespace::Array* arrayLocalizedText =
                static_cast<IODataProviderNamespace::Array*> (
                conv.convertUa2io(valueLocalizedText, UaNodeId(OpcUaType_LocalizedText)));
        CHECK_EQUAL(IODataProviderNamespace::Scalar::LOCALIZED_TEXT, arrayLocalizedText->getArrayType());
        const std::vector<const IODataProviderNamespace::Variant*>* elementsLocalizedText = arrayLocalizedText->getElements();
        CHECK_EQUAL(2, elementsLocalizedText->size());
        const IODataProviderNamespace::Scalar* elem =
                static_cast<const IODataProviderNamespace::Scalar*> (elementsLocalizedText->at(0));
        STRCMP_EQUAL("en", elem->getLocalizedTextLocale()->c_str());
        STRCMP_EQUAL("a", elem->getLocalizedTextText()->c_str());
        elem =
                static_cast<const IODataProviderNamespace::Scalar*> (elementsLocalizedText->at(1));
        STRCMP_EQUAL("de", elem->getLocalizedTextLocale()->c_str());
        STRCMP_EQUAL("b", elem->getLocalizedTextText()->c_str());
        delete arrayLocalizedText;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, UaVariantStructureArrayValue) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // create structure definitions and add them to the callback
        int nsIndex = 2;

        int Ba_ControllerConfiguration = 2;
        int Ba_ControllerConfiguration_DefaultBinary = 3;
        int Ba_ControllerConfiguration_DefaultXml = 4;
        int Ba_AirConditionerControllerConfiguration = 5;
        int Ba_AirConditionerControllerConfiguration_DefaultBinary = 6;
        int Ba_AirConditionerControllerConfiguration_DefaultXml = 7;
        int Ba_Configurations = 8;
        int Ba_Configurations_DefaultBinary = 9;
        int Ba_Configurations_DefaultXml = 10;

        UaStructureDefinition controllerConfiguration;
        controllerConfiguration.setName("ControllerConfiguration");
        controllerConfiguration.setDataTypeId(UaNodeId(Ba_ControllerConfiguration, nsIndex));
        controllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultBinary, nsIndex));
        controllerConfiguration.setXmlEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultXml, nsIndex));

        UaStructureField controllerConfigurationField;
        controllerConfigurationField.setName("Name");
        controllerConfigurationField.setDataTypeId(OpcUaId_String);
        controllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
        controllerConfiguration.addChild(controllerConfigurationField);
        controllerConfigurationField.setName("DeviceAddress");
        controllerConfigurationField.setDataTypeId(OpcUaId_UInt32);
        controllerConfiguration.addChild(controllerConfigurationField);
        controllerConfigurationField.setName("TemperatureSetpoint");
        controllerConfigurationField.setDataTypeId(OpcUaId_Double);
        controllerConfigurationField.setOptional(true);
        controllerConfiguration.addChild(controllerConfigurationField);

        callback.addStructureDefinition(controllerConfiguration);

        std::vector<UaNodeId> parentsControllerConfiguration;
        parentsControllerConfiguration.push_back(UaNodeId(OpcUaId_Structure));
        UaNodeId dataTypeControllerConfiguration(controllerConfiguration.dataTypeId());
        callback.addDataTypeParents(dataTypeControllerConfiguration,
                parentsControllerConfiguration);

        UaStructureDefinition airConditionerControllerConfiguration
                = controllerConfiguration.createSubtype();
        airConditionerControllerConfiguration.setName("AirConditionerControllerConfiguration");
        airConditionerControllerConfiguration.setDataTypeId(UaNodeId(Ba_AirConditionerControllerConfiguration, nsIndex));
        airConditionerControllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultBinary, nsIndex));
        airConditionerControllerConfiguration.setXmlEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultXml, nsIndex));

        UaStructureField airConditionerControllerConfigurationField;
        airConditionerControllerConfigurationField.setName("HumiditySetpoint");
        airConditionerControllerConfigurationField.setDataTypeId(OpcUaId_Double);
        airConditionerControllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
        airConditionerControllerConfiguration.addChild(airConditionerControllerConfigurationField);

        callback.addStructureDefinition(airConditionerControllerConfiguration);

        std::vector<UaNodeId> parentsAirConditionerControllerConfiguration;
        parentsAirConditionerControllerConfiguration.push_back(UaNodeId(OpcUaId_Structure));
        UaNodeId dataTypeAirConditionerControllerConfiguration(
                airConditionerControllerConfiguration.dataTypeId());
        callback.addDataTypeParents(dataTypeAirConditionerControllerConfiguration,
                parentsAirConditionerControllerConfiguration);

        UaStructureDefinition baConfigurations;
        baConfigurations.setName("BAConfigurations");
        baConfigurations.setDataTypeId(UaNodeId(Ba_Configurations, nsIndex));
        baConfigurations.setBinaryEncodingId(UaNodeId(Ba_Configurations_DefaultBinary, nsIndex));
        baConfigurations.setXmlEncodingId(UaNodeId(Ba_Configurations_DefaultXml, nsIndex));

        UaStructureField baConfigurationsField;
        baConfigurationsField.setName("AirConditionerControllers");
        baConfigurationsField.setStructureDefinition(airConditionerControllerConfiguration);
        baConfigurationsField.setArrayType(UaStructureField::ArrayType_Array);
        baConfigurations.addChild(baConfigurationsField);
        baConfigurationsField.setName("FurnaceControllers");
        baConfigurationsField.setStructureDefinition(controllerConfiguration);
        baConfigurations.addChild(baConfigurationsField);

        callback.addStructureDefinition(baConfigurations);

        std::vector<UaNodeId> parentsBaConfigurations;
        parentsBaConfigurations.push_back(UaNodeId(OpcUaId_Structure));
        UaNodeId dataTypeBaConfigurations(baConfigurations.dataTypeId());
        callback.addDataTypeParents(dataTypeBaConfigurations, parentsBaConfigurations);

        // create UaVariant
        UaGenericStructureValue controller1Sv(controllerConfiguration);
        controller1Sv.setField(UaString("Name"), UaVariant(UaString("Controller")));
        controller1Sv.setField(UaString("DeviceAddress"), UaVariant(static_cast<OpcUa_UInt32> (3)));
        UaExtensionObject controller1Eo;
        controller1Sv.toExtensionObject(controller1Eo);

        UaGenericStructureValue airConditionerController1Sv(airConditionerControllerConfiguration);
        airConditionerController1Sv.setField(UaString("Name"),
                UaVariant(UaString("AirConditionerController")));
        airConditionerController1Sv.setField(UaString("DeviceAddress"),
                UaVariant(static_cast<OpcUa_UInt32> (5)));
        airConditionerController1Sv.setField(UaString("TemperatureSetpoint"),
                UaVariant(static_cast<OpcUa_Double> (6.6)));
        airConditionerController1Sv.setField(UaString("HumiditySetpoint"),
                UaVariant(static_cast<OpcUa_Double> (7.7)));
        UaExtensionObject airConditionerController1Eo;
        airConditionerController1Sv.toExtensionObject(airConditionerController1Eo);

        UaExtensionObjectArray controllerArray;
        controllerArray.create(1);
        controller1Eo.copyTo(&controllerArray[0]);

        UaExtensionObjectArray airConditionerControllerArray;
        airConditionerControllerArray.create(1);
        airConditionerController1Eo.copyTo(&airConditionerControllerArray[0]);

        UaGenericStructureValue configurationsSv(baConfigurations);
        UaVariant tmp;
        tmp.setExtensionObjectArray(airConditionerControllerArray);
        configurationsSv.setField(UaString("AirConditionerControllers"), tmp);
        tmp.setExtensionObjectArray(controllerArray);
        configurationsSv.setField(UaString("FurnaceControllers"), tmp);
        UaExtensionObject configurationsEo;
        configurationsSv.toExtensionObject(configurationsEo);
        UaVariant value;
        value.setExtensionObject(configurationsEo, true /* detach*/);

        // convert UaVariant to Variant
        IODataProviderNamespace::Structure* configurationsStructure =
                static_cast<IODataProviderNamespace::Structure*> (
                conv.convertUa2io(value, UaNodeId(Ba_Configurations, nsIndex)));
        CHECK_EQUAL(Ba_Configurations, configurationsStructure->getDataTypeId().getNumeric());
        const std::map<std::string, const IODataProviderNamespace::Variant*>& configurationsFields
                = configurationsStructure->getFieldData();
        CHECK_EQUAL(2, configurationsFields.size());

        // check FurnaceControllers
        const IODataProviderNamespace::Array& controllers
                = *static_cast<const IODataProviderNamespace::Array*> (
                configurationsFields.at(std::string("FurnaceControllers")));
        CHECK_EQUAL(1, controllers.getElements()->size());
        const IODataProviderNamespace::Structure& controller1 =
                *static_cast<const IODataProviderNamespace::Structure*> (
                controllers.getElements()->at(0));
        CHECK_EQUAL(Ba_ControllerConfiguration, controller1.getDataTypeId().getNumeric());
        const std::map<std::string, const IODataProviderNamespace::Variant*>& controller1Fields
                = controller1.getFieldData();
        CHECK_EQUAL(2, controller1Fields.size());
        const IODataProviderNamespace::Scalar& controllerName =
                *static_cast<const IODataProviderNamespace::Scalar*> (
                controller1Fields.at(std::string("Name")));
        STRCMP_EQUAL("Controller", controllerName.getString()->c_str());
        const IODataProviderNamespace::Scalar& controllerDeviceAddress =
                *static_cast<const IODataProviderNamespace::Scalar*> (
                controller1Fields.at(std::string("DeviceAddress")));
        CHECK_EQUAL(3, controllerDeviceAddress.getULong());

        // check AirConditionerControllers
        const IODataProviderNamespace::Array& airConditionerControllers
                = *static_cast<const IODataProviderNamespace::Array*> (
                configurationsFields.at(std::string("AirConditionerControllers")));
        CHECK_EQUAL(1, airConditionerControllers.getElements()->size());
        const IODataProviderNamespace::Structure& airConditionerController1 =
                *static_cast<const IODataProviderNamespace::Structure*> (
                airConditionerControllers.getElements()->at(0));
        CHECK_EQUAL(Ba_AirConditionerControllerConfiguration,
                airConditionerController1.getDataTypeId().getNumeric());
        const std::map<std::string, const IODataProviderNamespace::Variant*>& airConditionerController1Fields
                = airConditionerController1.getFieldData();
        CHECK_EQUAL(4, airConditionerController1Fields.size());
        const IODataProviderNamespace::Scalar& airConditionerControllerName =
                *static_cast<const IODataProviderNamespace::Scalar*> (
                airConditionerController1Fields.at(std::string("Name")));
        STRCMP_EQUAL("AirConditionerController", airConditionerControllerName.getString()->c_str());
        const IODataProviderNamespace::Scalar& airConditionerControllerDeviceAddress =
                *static_cast<const IODataProviderNamespace::Scalar*> (
                airConditionerController1Fields.at(std::string("DeviceAddress")));
        CHECK_EQUAL(5, airConditionerControllerDeviceAddress.getULong());
        const IODataProviderNamespace::Scalar& airConditionerControllerTemperatureSetpoint =
                *static_cast<const IODataProviderNamespace::Scalar*> (
                airConditionerController1Fields.at(std::string("TemperatureSetpoint")));
        DOUBLES_EQUAL(6.6, airConditionerControllerTemperatureSetpoint.getDouble(), 0.1);
        const IODataProviderNamespace::Scalar& airConditionerControllerHumiditySetpoint =
                *static_cast<const IODataProviderNamespace::Scalar*> (
                airConditionerController1Fields.at(std::string("HumiditySetpoint")));
        DOUBLES_EQUAL(7.7, airConditionerControllerHumiditySetpoint.getDouble(), 0.1);

        delete configurationsStructure;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, VariantScalarValue) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // InternalInterface - Server
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

        // bool
        IODataProviderNamespace::Scalar s;
        s.setBool(true);
        UaVariant* value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Boolean));
        CHECK_EQUAL(OpcUaType_Boolean, value->dataType().identifierNumeric());
        OpcUa_Boolean valueBoolean;
        value->toBool(valueBoolean);
        CHECK_TRUE(valueBoolean);
        delete value;

        // schar
        s.setSChar(2);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_SByte));
        CHECK_EQUAL(OpcUaType_SByte, value->dataType().identifierNumeric());
        OpcUa_SByte valueSByte;
        value->toSByte(valueSByte);
        CHECK_EQUAL(2, valueSByte);
        delete value;

        // char
        s.setChar('a');
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Byte));
        CHECK_EQUAL(OpcUaType_Byte, value->dataType().identifierNumeric());
        OpcUa_Byte valueByte;
        value->toByte(valueByte);
        CHECK_EQUAL('a', valueByte);
        delete value;

        // int
        s.setInt(3);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Int16));
        CHECK_EQUAL(OpcUaType_Int16, value->dataType().identifierNumeric());
        OpcUa_Int16 valueInt16;
        value->toInt16(valueInt16);
        CHECK_EQUAL(3, valueInt16);
        delete value;

        // uint
        s.setUInt(4);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_UInt16));
        CHECK_EQUAL(OpcUaType_UInt16, value->dataType().identifierNumeric());
        OpcUa_UInt16 valueUInt16;
        value->toUInt16(valueUInt16);
        CHECK_EQUAL(4, valueUInt16);
        delete value;

        // long -> int32
        s.setLong(5);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Int32));
        CHECK_EQUAL(OpcUaType_Int32, value->dataType().identifierNumeric());
        OpcUa_Int32 valueInt32;
        value->toInt32(valueInt32);
        CHECK_EQUAL(5, valueInt32);
        delete value;

        // long -> uint16
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_UInt16));
        CHECK_EQUAL(OpcUaType_UInt16, value->dataType().identifierNumeric());
        value->toUInt16(valueUInt16);
        CHECK_EQUAL(5, valueUInt16);
        delete value;

        // long -> enum
        std::vector<UaNodeId> enumParents;
        enumParents.push_back(UaNodeId(OpcUaId_Enumeration));
        UaNodeId enumDataTypeId(34, 2 /* nsIndex */);
        callback.addDataTypeParents(enumDataTypeId, enumParents);

        value = conv.convertIo2ua(s, enumDataTypeId);
        CHECK_EQUAL(OpcUaType_Int32, value->dataType().identifierNumeric());
        value->toInt32(valueInt32);
        CHECK_EQUAL(5, valueInt32);
        delete value;

        // ulong
        s.setULong(6);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_UInt32));
        CHECK_EQUAL(OpcUaType_UInt32, value->dataType().identifierNumeric());
        OpcUa_UInt32 valueUInt32;
        value->toUInt32(valueUInt32);
        CHECK_EQUAL(6, valueUInt32);
        delete value;

        // llong -> int64
        s.setLLong(7);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Int64));
        CHECK_EQUAL(OpcUaType_Int64, value->dataType().identifierNumeric());
        OpcUa_Int64 valueInt64;
        value->toInt64(valueInt64);
        // cannot use CHECK_TRUE due to ARM compiler
        CHECK_TRUE(7 == valueInt64);
        delete value;

        // llong -> uint32
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_UInt32));
        CHECK_EQUAL(OpcUaType_UInt32, value->dataType().identifierNumeric());
        value->toUInt32(valueUInt32);
        CHECK_EQUAL(7, valueUInt32);
        delete value;

        // llong -> dateTime
        // 129828438100000000 / 10 / 1000 / 1000 / 86400 / ~365.25 
        // -> 411... years since 1601-01-01
        s.setLLong(129828438100000000);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_DateTime));
        CHECK_EQUAL(OpcUaType_DateTime, value->dataType().identifierNumeric());
        UaDateTime valueDateTime;
        value->toDateTime(valueDateTime);
        STRCMP_EQUAL("2012-05-30T09:30:10.000Z", valueDateTime.toString().toUtf8());
        delete value;

        // llong -> utcTime
        // 129828438100000000 / 10 / 1000 / 1000 / 86400 / ~365.25 
        // -> 411... years since 1601-01-01       
        s.setLLong(129828438100000000);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaId_UtcTime));
        CHECK_EQUAL(OpcUaType_DateTime, value->dataType().identifierNumeric());
        value->toDateTime(valueDateTime);
        STRCMP_EQUAL("2012-05-30T09:30:10.000Z", valueDateTime.toString().toUtf8());
        delete value;

        // ullong
        s.setULLong(8);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_UInt64));
        CHECK_EQUAL(OpcUaType_UInt64, value->dataType().identifierNumeric());
        OpcUa_UInt64 valueUInt64;
        value->toUInt64(valueUInt64);
        // cannot use CHECK_TRUE due to ARM compiler
        CHECK_TRUE(8 == valueUInt64);
        delete value;

        // float                
        s.setFloat(9.9);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Float));
        CHECK_EQUAL(OpcUaType_Float, value->dataType().identifierNumeric());
        OpcUa_Float valueFloat;
        value->toFloat(valueFloat);
        DOUBLES_EQUAL(9.9, valueFloat, 0.1);
        delete value;

        // double        
        s.setDouble(10.1);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_Double));
        CHECK_EQUAL(OpcUaType_Double, value->dataType().identifierNumeric());
        OpcUa_Double valueDouble;
        value->toDouble(valueDouble);
        DOUBLES_EQUAL(10.1, valueDouble, 0.1);
        delete value;

        // duration
        s.setDouble(10.1);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaId_Duration));
        CHECK_EQUAL(OpcUaType_Double, value->dataType().identifierNumeric());
        value->toDouble(valueDouble);
        DOUBLES_EQUAL(10.1, valueDouble, 0.1);
        delete value;

        // string -> string
        std::string valueStr("a");
        s.setString(&valueStr);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_String));
        CHECK_EQUAL(OpcUaType_String, value->dataType().identifierNumeric());
        UaString valueString = value->toString();
        STRCMP_EQUAL("a", valueString.toUtf8());
        delete value;

        IODataProviderNamespace::Scalar scalarStringNull;
        scalarStringNull.setString(NULL);
        value = conv.convertIo2ua(scalarStringNull, UaNodeId(OpcUaType_String));
        CHECK_EQUAL(OpcUaType_String, value->dataType().identifierNumeric());
        CHECK_TRUE(value->toString().isNull());
        delete value;

        // string -> localizedText
        std::string strlocalizedTextText("a");
        s.setString(&strlocalizedTextText);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_LocalizedText));
        CHECK_EQUAL(OpcUaType_LocalizedText, value->dataType().identifierNumeric());
        UaLocalizedText strLocalizedTextValue;
        value->toLocalizedText(strLocalizedTextValue);
        STRCMP_EQUAL("en", UaString(*strLocalizedTextValue.locale()).toUtf8());
        STRCMP_EQUAL("a", UaString(*strLocalizedTextValue.text()).toUtf8());
        delete value;

        s.setString(NULL);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_LocalizedText));
        CHECK_EQUAL(OpcUaType_LocalizedText, value->dataType().identifierNumeric());
        UaLocalizedText strLocalizedTextValue2;
        value->toLocalizedText(strLocalizedTextValue2);
        CHECK_TRUE(strLocalizedTextValue2.isNull());
        delete value;

        // byteString
        char valueByteStr[2];
        valueByteStr[0] = 34;
        valueByteStr[1] = 255;
        s.setByteString(valueByteStr, 2 /*length*/);
        value = static_cast<UaVariant*> (conv.convertIo2ua(s, UaNodeId(OpcUaType_ByteString)));
        CHECK_EQUAL(OpcUaType_ByteString, value->type());
        UaByteString byteString;
        value->toByteString(byteString);
        CHECK_EQUAL(2, byteString.length());
        const OpcUa_Byte* bs = byteString.data();
        CHECK_EQUAL(34, bs[0] & 0xFF);
        CHECK_EQUAL(255, bs[1] & 0xFF);
        delete value;

        s.setByteString(NULL, -1 /*length*/);
        value = static_cast<UaVariant*> (conv.convertIo2ua(s, UaNodeId(OpcUaType_ByteString)));
        CHECK_EQUAL(OpcUaType_ByteString, value->type());
        UaByteString byteStringNull;
        value->toByteString(byteStringNull);
        CHECK_EQUAL(-1, byteStringNull.length());
        CHECK_TRUE(byteStringNull.data() == NULL);
        delete value;

        // localizedText
        std::string localizedTextLocale("a");
        std::string localizedTextText("b");
        s.setLocalizedText(&localizedTextLocale, &localizedTextText, false /* attachValues*/);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_LocalizedText));
        CHECK_EQUAL(OpcUaType_LocalizedText, value->dataType().identifierNumeric());
        UaLocalizedText localizedTextValue;
        value->toLocalizedText(localizedTextValue);
        STRCMP_EQUAL("a", UaString(*localizedTextValue.locale()).toUtf8());
        STRCMP_EQUAL("b", UaString(*localizedTextValue.text()).toUtf8());
        delete value;

        s.setLocalizedText(NULL, NULL, false /* attachValues*/);
        value = conv.convertIo2ua(s, UaNodeId(OpcUaType_LocalizedText));
        CHECK_EQUAL(OpcUaType_LocalizedText, value->dataType().identifierNumeric());
        UaLocalizedText localizedTextValue2;
        value->toLocalizedText(localizedTextValue2);
        CHECK_TRUE(localizedTextValue2.isNull());
        delete value;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, VariantScalarValueError) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // bool
        IODataProviderNamespace::Scalar s;
        s.setBool(true);
        try {
            conv.convertIo2ua(s, UaNodeId(OpcUaType_Int32));
            FAIL("");
        } catch (ConversionException& e) {
            STRCMP_CONTAINS("Scalar of type 1 to UaVariant of type i=6", e.getMessage().c_str());
        } catch (Exception& e) {
            FAIL("");
        }

    }

    TEST(SasModelProviderBase_ConverterUa2IO, VariantArrayValue) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // InternalInterface      - Server
        // long[]                 - int32[]/uint16[]/enum[]
        // ulong[]                - uint32[]
        // string[]               - string[]/localizedText[]
        // localizedText[]        - localizedText[]

        // long[] -> int32[]
        std::vector<const IODataProviderNamespace::Variant*>* elements
                = new std::vector<const IODataProviderNamespace::Variant*>();
        IODataProviderNamespace::Scalar* s = new IODataProviderNamespace::Scalar();
        s->setLong(3);
        elements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setLong(4);
        elements->push_back(s);
        IODataProviderNamespace::Array arrayInt32(IODataProviderNamespace::Scalar::LONG, elements,
                true /*attachValues*/);
        // convert Variant to UaVariant
        UaVariant* value = conv.convertIo2ua(arrayInt32, UaNodeId(OpcUaType_Int32));
        CHECK_EQUAL(OpcUaType_Int32, value->dataType().identifierNumeric());
        UaInt32Array valueInt32Array;
        value->toInt32Array(valueInt32Array);
        CHECK_EQUAL(2, valueInt32Array.length());
        CHECK_EQUAL(3, valueInt32Array[0]);
        CHECK_EQUAL(4, valueInt32Array[1]);
        delete value;

        // long[] -> uint16[]
        elements = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setLong(7);
        elements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setLong(8);
        elements->push_back(s);
        IODataProviderNamespace::Array arrayUInt16(IODataProviderNamespace::Scalar::LONG, elements,
                true /*attachValues*/);
        // convert Variant to UaVariant
        value = conv.convertIo2ua(arrayUInt16, UaNodeId(OpcUaType_UInt16));
        CHECK_EQUAL(OpcUaType_UInt16, value->dataType().identifierNumeric());
        UaUInt16Array valueUInt16Array;
        value->toUInt16Array(valueUInt16Array);
        CHECK_EQUAL(2, valueUInt16Array.length());
        CHECK_EQUAL(7, valueUInt16Array[0]);
        CHECK_EQUAL(8, valueUInt16Array[1]);
        delete value;

        // long[] -> enum[]
        std::vector<UaNodeId> enumParents;
        enumParents.push_back(UaNodeId(OpcUaId_Enumeration));
        UaNodeId enumDataTypeId(34, 2 /* nsIndex */);
        callback.addDataTypeParents(enumDataTypeId, enumParents);

        elements = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setLong(10);
        elements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setLong(11);
        elements->push_back(s);
        IODataProviderNamespace::Array arrayEnum(IODataProviderNamespace::Scalar::LONG, elements,
                true /*attachValues*/);
        // convert Variant to UaVariant
        value = conv.convertIo2ua(arrayEnum, enumDataTypeId);
        CHECK_EQUAL(OpcUaType_Int32, value->dataType().identifierNumeric());
        value->toInt32Array(valueInt32Array);
        CHECK_EQUAL(2, valueInt32Array.length());
        CHECK_EQUAL(10, valueInt32Array[0]);
        CHECK_EQUAL(11, valueInt32Array[1]);
        delete value;

        // ulong[] -> uint32[]
        elements = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setULong(3);
        elements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setULong(4);
        elements->push_back(s);
        IODataProviderNamespace::Array arrayUInt32(IODataProviderNamespace::Scalar::ULONG, elements,
                true /*attachValues*/);
        // convert Variant to UaVariant
        value = conv.convertIo2ua(arrayUInt32, UaNodeId(OpcUaType_UInt32));
        CHECK_EQUAL(OpcUaType_UInt32, value->dataType().identifierNumeric());
        UaUInt32Array valueUInt32Array;
        value->toUInt32Array(valueUInt32Array);
        CHECK_EQUAL(2, valueUInt32Array.length());
        CHECK_EQUAL(3, valueUInt32Array[0]);
        CHECK_EQUAL(4, valueUInt32Array[1]);
        delete value;

        // string[] -> string[]
        elements = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setString(new std::string("a"), true /*attachValues*/);
        elements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setString(new std::string("b"), true /*attachValues*/);
        elements->push_back(s);
        IODataProviderNamespace::Array arrayString(IODataProviderNamespace::Scalar::STRING, elements,
                true /*attachValues*/);
        // convert Variant to UaVariant
        value = conv.convertIo2ua(arrayString, UaNodeId(OpcUaType_String));
        CHECK_EQUAL(OpcUaType_String, value->dataType().identifierNumeric());
        UaStringArray valueStringArray;
        value->toStringArray(valueStringArray);
        CHECK_EQUAL(2, valueStringArray.length());
        STRCMP_EQUAL("a", UaString(valueStringArray[0]).toUtf8());
        STRCMP_EQUAL("b", UaString(valueStringArray[1]).toUtf8());
        delete value;
        
        // string[] -> localizedText[]                
        value = conv.convertIo2ua(arrayString, UaNodeId(OpcUaType_LocalizedText));
        UaLocalizedTextArray valueLocalizedTextArray;
        value->toLocalizedTextArray(valueLocalizedTextArray);
        CHECK_EQUAL(2, valueLocalizedTextArray.length());
        STRCMP_EQUAL("en", UaString(valueLocalizedTextArray[0].Locale).toUtf8());
        STRCMP_EQUAL("a", UaString(valueLocalizedTextArray[0].Text).toUtf8());
        STRCMP_EQUAL("en", UaString(valueLocalizedTextArray[1].Locale).toUtf8());
        STRCMP_EQUAL("b", UaString(valueLocalizedTextArray[1].Text).toUtf8());
        delete value;
        
        // localizedText[] -> localizedText[]
        elements = new std::vector<const IODataProviderNamespace::Variant*>();
        s = new IODataProviderNamespace::Scalar();
        s->setLocalizedText(new std::string("en"), new std::string("a"), true /*attachValues*/);
        elements->push_back(s);
        s = new IODataProviderNamespace::Scalar();
        s->setLocalizedText(new std::string("de"), new std::string("b"), true /*attachValues*/);
        elements->push_back(s);
        IODataProviderNamespace::Array arrayLocalizedText(IODataProviderNamespace::Scalar::LOCALIZED_TEXT, elements,
                true /*attachValues*/);
        // convert Variant to UaVariant
        value = conv.convertIo2ua(arrayLocalizedText, UaNodeId(OpcUaType_LocalizedText));
        CHECK_EQUAL(OpcUaType_LocalizedText, value->dataType().identifierNumeric());        
        value->toLocalizedTextArray(valueLocalizedTextArray);
        CHECK_EQUAL(2, valueLocalizedTextArray.length());
        STRCMP_EQUAL("en", UaString(valueLocalizedTextArray[0].Locale).toUtf8());
        STRCMP_EQUAL("a", UaString(valueLocalizedTextArray[0].Text).toUtf8());
        STRCMP_EQUAL("de", UaString(valueLocalizedTextArray[1].Locale).toUtf8());
        STRCMP_EQUAL("b", UaString(valueLocalizedTextArray[1].Text).toUtf8());
        delete value;
    }

    TEST(SasModelProviderBase_ConverterUa2IO, VariantStructureArrayValue) {
        ConverterCallback callback;
        ConverterUa2IO conv(callback);

        // create structure definitions and add them to the callback
        int Ba_ControllerConfiguration = 2;
        int Ba_ControllerConfiguration_DefaultBinary = 3;
        int Ba_ControllerConfiguration_DefaultXml = 4;
        int Ba_AirConditionerControllerConfiguration = 5;
        int Ba_AirConditionerControllerConfiguration_DefaultBinary = 6;
        int Ba_AirConditionerControllerConfiguration_DefaultXml = 7;
        int Ba_Configurations = 8;
        int Ba_Configurations_DefaultBinary = 9;
        int Ba_Configurations_DefaultXml = 10;

        int nsIndex = 2;

        UaStructureDefinition controllerConfiguration;
        controllerConfiguration.setName("ControllerConfiguration");
        controllerConfiguration.setDataTypeId(UaNodeId(Ba_ControllerConfiguration, nsIndex));
        controllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultBinary, nsIndex));
        controllerConfiguration.setXmlEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultXml, nsIndex));

        UaStructureField controllerConfigurationField;
        controllerConfigurationField.setName("Name");
        controllerConfigurationField.setDataTypeId(OpcUaId_String);
        controllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
        controllerConfiguration.addChild(controllerConfigurationField);
        controllerConfigurationField.setName("DeviceAddress");
        controllerConfigurationField.setDataTypeId(OpcUaId_UInt32);
        controllerConfiguration.addChild(controllerConfigurationField);
        controllerConfigurationField.setName("TemperatureSetpoint");
        controllerConfigurationField.setDataTypeId(OpcUaId_Double);
        controllerConfigurationField.setOptional(true);
        controllerConfiguration.addChild(controllerConfigurationField);

        callback.addStructureDefinition(controllerConfiguration);

        std::vector<UaNodeId> parentsControllerConfiguration;
        parentsControllerConfiguration.push_back(UaNodeId(OpcUaId_Structure));
        UaNodeId dataTypeControllerConfiguration(controllerConfiguration.dataTypeId());
        callback.addDataTypeParents(dataTypeControllerConfiguration,
                parentsControllerConfiguration);

        UaStructureDefinition airConditionerControllerConfiguration
                = controllerConfiguration.createSubtype();
        airConditionerControllerConfiguration.setName("AirConditionerControllerConfiguration");
        airConditionerControllerConfiguration.setDataTypeId(UaNodeId(Ba_AirConditionerControllerConfiguration, nsIndex));
        airConditionerControllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultBinary, nsIndex));
        airConditionerControllerConfiguration.setXmlEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultXml, nsIndex));

        UaStructureField airConditionerControllerConfigurationField;
        airConditionerControllerConfigurationField.setName("HumiditySetpoint");
        airConditionerControllerConfigurationField.setDataTypeId(OpcUaId_Double);
        airConditionerControllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
        airConditionerControllerConfiguration.addChild(airConditionerControllerConfigurationField);

        callback.addStructureDefinition(airConditionerControllerConfiguration);

        std::vector<UaNodeId> parentsAirConditionerControllerConfiguration;
        parentsAirConditionerControllerConfiguration.push_back(UaNodeId(OpcUaId_Structure));
        UaNodeId dataTypeAirConditionerControllerConfiguration(
                airConditionerControllerConfiguration.dataTypeId());
        callback.addDataTypeParents(dataTypeAirConditionerControllerConfiguration,
                parentsAirConditionerControllerConfiguration);

        UaStructureDefinition baConfigurations;
        baConfigurations.setName("BAConfigurations");
        baConfigurations.setDataTypeId(UaNodeId(Ba_Configurations, nsIndex));
        baConfigurations.setBinaryEncodingId(UaNodeId(Ba_Configurations_DefaultBinary, nsIndex));
        baConfigurations.setXmlEncodingId(UaNodeId(Ba_Configurations_DefaultXml, nsIndex));

        UaStructureField baConfigurationsField;
        baConfigurationsField.setName("AirConditionerControllers");
        baConfigurationsField.setStructureDefinition(airConditionerControllerConfiguration);
        baConfigurationsField.setArrayType(UaStructureField::ArrayType_Array);
        baConfigurations.addChild(baConfigurationsField);
        baConfigurationsField.setName("FurnaceControllers");
        baConfigurationsField.setStructureDefinition(controllerConfiguration);
        baConfigurations.addChild(baConfigurationsField);

        callback.addStructureDefinition(baConfigurations);

        std::vector<UaNodeId> parentsBaConfigurations;
        parentsBaConfigurations.push_back(UaNodeId(OpcUaId_Structure));
        UaNodeId dataTypeBaConfigurations(baConfigurations.dataTypeId());
        callback.addDataTypeParents(dataTypeBaConfigurations, parentsBaConfigurations);

        // create Variant
        std::map<std::string, const IODataProviderNamespace::Variant*> controllerFieldData;
        std::string cn("Controller");
        IODataProviderNamespace::Scalar controllerName;
        controllerName.setString(&cn);
        controllerFieldData[std::string("Name")] = &controllerName;
        IODataProviderNamespace::Scalar controllerDeviceAddress;
        controllerDeviceAddress.setLLong(3);
        controllerFieldData[std::string("DeviceAddress")] = &controllerDeviceAddress;
        IODataProviderNamespace::NodeId controllerDataTypeId(nsIndex, Ba_ControllerConfiguration);
        const IODataProviderNamespace::Structure controllerStructure(controllerDataTypeId, controllerFieldData);

        std::map<std::string, const IODataProviderNamespace::Variant*> airConditionerControllerFieldData;
        std::string acn("AirConditionerController");
        IODataProviderNamespace::Scalar airConditionerControllerName;
        airConditionerControllerName.setString(&acn);
        airConditionerControllerFieldData[std::string("Name")] = &airConditionerControllerName;
        IODataProviderNamespace::Scalar airConditionerControllerDeviceAddress;
        airConditionerControllerDeviceAddress.setLLong(5);
        airConditionerControllerFieldData[std::string("DeviceAddress")] = &airConditionerControllerDeviceAddress;
        IODataProviderNamespace::Scalar airConditionerControllerTemperatureSetpoint;
        airConditionerControllerTemperatureSetpoint.setDouble(6.6);
        airConditionerControllerFieldData[std::string("TemperatureSetpoint")] = &airConditionerControllerTemperatureSetpoint;
        IODataProviderNamespace::Scalar airConditionerControllerHumiditySetpoint;
        airConditionerControllerHumiditySetpoint.setDouble(7.7);
        airConditionerControllerFieldData[std::string("HumiditySetpoint")] = &airConditionerControllerHumiditySetpoint;
        IODataProviderNamespace::NodeId airConditionerControllerDataTypeId(nsIndex, Ba_AirConditionerControllerConfiguration);
        IODataProviderNamespace::Structure airConditionerControllerStructure(airConditionerControllerDataTypeId, airConditionerControllerFieldData);

        std::vector<const IODataProviderNamespace::Variant*> controllerStructureArrayElements;
        controllerStructureArrayElements.push_back(&controllerStructure);
        const IODataProviderNamespace::Array controllerStructureArray(IODataProviderNamespace::Variant::STRUCTURE,
                &controllerStructureArrayElements);

        std::vector<const IODataProviderNamespace::Variant*> airConditionerControllerStructureArrayElements;
        airConditionerControllerStructureArrayElements.push_back(&airConditionerControllerStructure);
        const IODataProviderNamespace::Array airConditionerControllerStructureArray(IODataProviderNamespace::Variant::STRUCTURE,
                &airConditionerControllerStructureArrayElements);

        std::map<std::string, const IODataProviderNamespace::Variant*> configurationsFieldData;
        configurationsFieldData[std::string("AirConditionerControllers")] = &airConditionerControllerStructureArray;
        configurationsFieldData[std::string("FurnaceControllers")] = &controllerStructureArray;
        IODataProviderNamespace::NodeId configurationsDataTypeId(nsIndex, Ba_Configurations);
        IODataProviderNamespace::Structure configurationsStructure(configurationsDataTypeId, configurationsFieldData);

        // convert Variant to UaVariant
        UaVariant* configurationsValue = conv.convertIo2ua(configurationsStructure,
                UaNodeId(Ba_Configurations, nsIndex));
        CHECK_EQUAL(OpcUaType_ExtensionObject, configurationsValue->type());
        UaExtensionObject configurationsEo;
        configurationsValue->toExtensionObject(configurationsEo);
        UaGenericStructureValue configurationsSv(configurationsEo, baConfigurations);

        // check FurnaceControllers
        OpcUa_StatusCode status;
        UaVariant controllersVa = configurationsSv.value(UaString("FurnaceControllers"), &status);
        CHECK_TRUE(controllersVa.isArray());
        CHECK_EQUAL(1, controllersVa.arraySize());
        OpcUa_UInt32 index = 0;
        OpcUa_Variant controller1V = controllersVa[index];

        UaVariant controller1Value(controller1V);
        CHECK_EQUAL(OpcUaType_ExtensionObject, controller1Value.type());
        UaExtensionObject controller1Eo;
        controller1Value.toExtensionObject(controller1Eo);
        UaGenericStructureValue controller1Sv(controller1Eo, controllerConfiguration);

        STRCMP_EQUAL("Controller", controller1Sv.value(
                UaString("Name"), &status).toString().toUtf8());
        OpcUa_UInt32 uint32Value;
        controller1Sv.value(UaString("DeviceAddress")).toUInt32(uint32Value);
        CHECK_EQUAL(3, uint32Value);
        CHECK_FALSE(controller1Sv.isFieldSet(UaString("TemperatureSetpoint")));

        // check AirConditionerControllers
        UaVariant airConditionerControllersVa =
                configurationsSv.value(UaString("AirConditionerControllers"), &status);
        CHECK_TRUE(airConditionerControllersVa.isArray());
        CHECK_EQUAL(1, airConditionerControllersVa.arraySize());
        index = 0;
        OpcUa_Variant airConditionerController1V = airConditionerControllersVa[index];

        UaVariant airConditionerController1Value(airConditionerController1V);
        CHECK_EQUAL(OpcUaType_ExtensionObject, airConditionerController1Value.type());
        UaExtensionObject airConditionerController1Eo;
        airConditionerController1Value.toExtensionObject(airConditionerController1Eo);
        UaGenericStructureValue airConditionerController1Sv(airConditionerController1Eo,
                airConditionerControllerConfiguration);

        STRCMP_EQUAL("AirConditionerController", airConditionerController1Sv.value(
                UaString("Name"), &status).toString().toUtf8());
        airConditionerController1Sv.value(UaString("DeviceAddress")).toUInt32(uint32Value);
        CHECK_EQUAL(5, uint32Value);
        OpcUa_Double doubleValue;
        airConditionerController1Sv.value(UaString("TemperatureSetpoint"), &status).toDouble(doubleValue);
        DOUBLES_EQUAL(6.6, doubleValue, 0.1);
        airConditionerController1Sv.value(UaString("HumiditySetpoint"), &status).toDouble(doubleValue);
        DOUBLES_EQUAL(7.7, doubleValue, 0.1);

        delete configurationsValue;
    }

}
