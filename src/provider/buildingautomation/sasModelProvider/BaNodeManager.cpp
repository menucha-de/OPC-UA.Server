#include "BaNodeManager.h"
#include "airconditionercontrollerobject.h"
#include "furnacecontrollerobject.h"
#include "../buildingautomationtypeids.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <opcua_analogitemtype.h>

using namespace CommonNamespace;
using namespace SASModelProviderNamespace;

class BaNodeManagerPrivate {
    friend class BaNodeManager;
private:
    Logger* log;
};

BaNodeManager::BaNodeManager(
        IODataProviderNamespace::IODataProvider& ioDataProvider) :
CodeNodeManagerBase(
"urn:UnifiedAutomation:CppDemoServer:BuildingAutomation",
ioDataProvider,
OpcUa_True /* firesEvents */) {
    d = new BaNodeManagerPrivate();
    d->log = LoggerFactory::getLogger("BaNodeManager");
}

BaNodeManager::~BaNodeManager() {
    delete d;
}

UaStatus BaNodeManager::afterStartUp() {
    UaStatus ret = CodeNodeManagerBase::afterStartUp();
    if (ret.isNotGood()) {
        return ret;
    }
    d->log->info("Started node manager for namespace %s with index %d",
            getNameSpaceUri().toUtf8(), getNameSpaceIndex());
    createTypeNodes();
    createObjectNodes();
    d->log->info("Created node set for namespace index %d", getNameSpaceIndex());
    return ret;
}

UaStatus BaNodeManager::createTypeNodes() {
    UaStatus ret;

    std::vector<UaObjectType*> objectTypes;

    UaUInt32Array nullarray;

    /**************************************************************
     * Create DataType nodes
     **************************************************************/
    addTypeDictionary(UaNodeId("ba", getNameSpaceIndex()), "ba");

    // Controller configuration DataTypes
    UaStructureDefinition controllerConfiguration;
    controllerConfiguration.setName("ControllerConfiguration");
    controllerConfiguration.setDataTypeId(UaNodeId(Ba_ControllerConfiguration, getNameSpaceIndex()));
    controllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultBinary, getNameSpaceIndex()));
    controllerConfiguration.setXmlEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultXml, getNameSpaceIndex()));

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
    //controllerConfigurationField.setOptional(true);
    controllerConfiguration.addChild(controllerConfigurationField);

    addStructuredType(controllerConfiguration);

    UaStructureDefinition airConditionerControllerConfiguration
            = controllerConfiguration.createSubtype();
    airConditionerControllerConfiguration.setName("AirConditionerControllerConfiguration");
    airConditionerControllerConfiguration.setDataTypeId(UaNodeId(Ba_AirConditionerControllerConfiguration, getNameSpaceIndex()));
    airConditionerControllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultBinary, getNameSpaceIndex()));
    airConditionerControllerConfiguration.setXmlEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultXml, getNameSpaceIndex()));

    UaStructureField airConditionerControllerConfigurationField;
    airConditionerControllerConfigurationField.setName("HumiditySetpoint");
    airConditionerControllerConfigurationField.setDataTypeId(OpcUaId_Double);
    airConditionerControllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
    airConditionerControllerConfiguration.addChild(airConditionerControllerConfigurationField);

    addStructuredType(airConditionerControllerConfiguration);

    UaStructureDefinition baConfigurations;
    baConfigurations.setName("BAConfigurations");
    baConfigurations.setDataTypeId(UaNodeId(Ba_Configurations, getNameSpaceIndex()));
    baConfigurations.setBinaryEncodingId(UaNodeId(Ba_Configurations_DefaultBinary, getNameSpaceIndex()));
    baConfigurations.setXmlEncodingId(UaNodeId(Ba_Configurations_DefaultXml, getNameSpaceIndex()));

    UaStructureField baConfigurationsField;
    baConfigurationsField.setName("AirConditionerControllers");
    baConfigurationsField.setStructureDefinition(airConditionerControllerConfiguration);
    baConfigurationsField.setArrayType(UaStructureField::ArrayType_Array);
    baConfigurations.addChild(baConfigurationsField);
    baConfigurationsField.setName("FurnaceControllers");
    baConfigurationsField.setStructureDefinition(controllerConfiguration);
    baConfigurations.addChild(baConfigurationsField);

    addStructuredType(baConfigurations);

    /**************************************************************
     * Create the Controller Type
     **************************************************************/
    // Add ObjectType "ControllerType"
    UaObjectTypeSimple* controllerType = new UaObjectTypeSimple("ControllerType", // Used as string in browse name and display name
            UaNodeId(Ba_ControllerType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId, // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True); // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    UaStatus addStatus = addNodeAndReference(OpcUaId_BaseObjectType, controllerType,
            OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Controller Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    UaVariant defaultValue;
    defaultValue.setUInt32(0); // BaCommunicationInterface::Ba_ControllerState_Off
    OpcUa::BaseDataVariableType* baseDataVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(Ba_ControllerType_State, getNameSpaceIndex()), // NodeId of the Variable
            "State", // Name of the Variable
            getNameSpaceIndex(), // Namespace index of the browse name (same like NodeId)
            defaultValue, // Initial value
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, // Access level
            this); // Node manager for this variable
    // Set Modelling Rule to Mandatory
    baseDataVariable->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, baseDataVariable,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "Temperature" as AnalogItem
    defaultValue.setDouble(0);
    OpcUa::AnalogItemType* analogItem = new OpcUa::AnalogItemType(
            UaNodeId(Ba_ControllerType_Temperature, getNameSpaceIndex()),
            "Temperature", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead, this);
    analogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, analogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    analogItem->setEURange(tempRange);
    UaEUInformation tempEUInformation("", -1,
            UaLocalizedText("en", "\xc2\xb0\x46") /* �F */,
            UaLocalizedText("en", "Degrees Fahrenheit"));
    analogItem->setEngineeringUnits(tempEUInformation);

    // Add Variable "TemperatureSetpoint" as AnalogItem
    defaultValue.setDouble(0);
    analogItem = new OpcUa::AnalogItemType(
            UaNodeId(Ba_ControllerType_TemperatureSetpoint,
            getNameSpaceIndex()), "TemperatureSetpoint",
            getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    analogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, analogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    analogItem->setEURange(tempRange);
    analogItem->setEngineeringUnits(tempEUInformation);

    // Add Variable "PowerConsumption"
    defaultValue.setDouble(0);
    OpcUa::DataItemType* dataItem = new OpcUa::DataItemType(
            UaNodeId(Ba_ControllerType_PowerConsumption, getNameSpaceIndex()),
            "PowerConsumption", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead, this);
    dataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, dataItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Start"
    OpcUa::BaseMethod* baseMethod = new OpcUa::BaseMethod(
            UaNodeId(Ba_ControllerType_Start, getNameSpaceIndex()), "Start",
            getNameSpaceIndex());
    baseMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, baseMethod,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    baseMethod = new OpcUa::BaseMethod(
            UaNodeId(Ba_ControllerType_Stop, getNameSpaceIndex()), "Stop",
            getNameSpaceIndex());
    baseMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, baseMethod,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "SetControllerConfigurations"
    baseMethod = new OpcUa::BaseMethod(
            UaNodeId(Ba_ControllerType_SetControllerConfigurations,
            getNameSpaceIndex()), "SetControllerConfigurations",
            getNameSpaceIndex());
    baseMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(controllerType, baseMethod,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    UaPropertyMethodArgument* propertyMethodArg = new UaPropertyMethodArgument(
            UaNodeId(Ba_ControllerType_SetControllerConfigurations_In, getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            1, // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument ControllerConfigurations
    propertyMethodArg->setArgument(0, // Index of the argument
            "ControllerConfigurations", // Name of the argument
            UaNodeId(Ba_Configurations, getNameSpaceIndex()), // Data type of the argument
            -1, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller configurations")); // Description
    // Add property to method
    addStatus = addNodeAndReference(baseMethod, propertyMethodArg,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
    // Add Property "OutputArguments"
    propertyMethodArg = new UaPropertyMethodArgument(
            UaNodeId(Ba_ControllerType_SetControllerConfigurations_Out, getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            1, // Number of arguments
            UaPropertyMethodArgument::OUTARGUMENTS);
    // Argument ControllerConfigurations
    propertyMethodArg->setArgument(0, // Index of the argument
            "ControllerConfigurations", // Name of the argument
            UaNodeId(Ba_Configurations, getNameSpaceIndex()), // Data type of the argument
            -1, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller configurations")); // Description
    // Add property to method
    addStatus = addNodeAndReference(baseMethod, propertyMethodArg,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the AirConditionerController Type
     **************************************************************/
    UaObjectTypeSimple* airConditionerControllerType = new UaObjectTypeSimple(
            "AirConditionerControllerType", // Used as string in browse name and display name
            UaNodeId(Ba_AirConditionerControllerType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,
            OpcUa_False);
    // Add Object Type node to address space and create reference to Controller type
    addStatus = addNodeAndReference(controllerType,
            airConditionerControllerType,
            OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the AirConditionerController Type Instance declaration
     ***************************************************************/
    // Add Variable "Humidity"
    defaultValue.setDouble(0);
    analogItem = new OpcUa::AnalogItemType(
            UaNodeId(Ba_AirConditionerControllerType_Humidity,
            getNameSpaceIndex()), "Humidity", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    analogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(airConditionerControllerType, analogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    analogItem->setEURange(tempRange);
    UaEUInformation tempEUPercent(
            "http://www.opcfoundation.org/UA/units/un/cefact", 20529,
            UaLocalizedText("en", "%"), UaLocalizedText("en", "percent"));
    analogItem->setEngineeringUnits(tempEUPercent);

    // Add Variable "HumiditySetpoint"
    defaultValue.setDouble(0);
    analogItem = new OpcUa::AnalogItemType(
            UaNodeId(Ba_AirConditionerControllerType_HumiditySetpoint,
            getNameSpaceIndex()), "HumiditySetpoint",
            getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    analogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(airConditionerControllerType, analogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    analogItem->setEURange(tempRange);
    analogItem->setEngineeringUnits(tempEUPercent);

    // Add Method "StartWithSetpoint"
    baseMethod = new OpcUa::BaseMethod(
            UaNodeId(Ba_AirConditionerControllerType_StartWithSetpoint,
            getNameSpaceIndex()), "StartWithSetpoint",
            getNameSpaceIndex());
    baseMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(airConditionerControllerType, baseMethod,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    propertyMethodArg = new UaPropertyMethodArgument(UaNodeId(
            Ba_AirConditionerControllerType_StartWithSetpoint_In, getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            2, // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetpoint
    propertyMethodArg->setArgument(0, // Index of the argument
            "TemperatureSetpoint", // Name of the argument
            UaNodeId(OpcUaId_Double), // Data type of the argument
            -1, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Argument HumiditySetpoint
    propertyMethodArg->setArgument(1, // Index of the argument
            "HumiditySetpoint", // Name of the argument
            UaNodeId(OpcUaId_Double), // Data type of the argument
            -1, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller set point for humidity")); // Description
    // Add property to method
    addStatus = addNodeAndReference(baseMethod, propertyMethodArg,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the FurnaceController Type
     **************************************************************/
    UaObjectTypeSimple* furnaceControllerType = new UaObjectTypeSimple("FurnaceControllerType", // Used as string in browse name and display name
            UaNodeId(Ba_FurnaceControllerType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,
            OpcUa_False);
    // Add Object Type node to address space and create reference to Controller type
    addStatus = addNodeAndReference(controllerType, furnaceControllerType,
            OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the FurnaceController Type Instance declaration
     **************************************************************/
    // Add Variable "GasFlow"
    defaultValue.setDouble(0);
    dataItem = new OpcUa::DataItemType(
            UaNodeId(Ba_FurnaceControllerType_GasFlow, getNameSpaceIndex()),
            "GasFlow", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead, this);
    dataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(furnaceControllerType, dataItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "StartWithSetpoint"
    baseMethod = new OpcUa::BaseMethod(
            UaNodeId(Ba_FurnaceControllerType_StartWithSetpoint,
            getNameSpaceIndex()), "StartWithSetpoint",
            getNameSpaceIndex());
    baseMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(furnaceControllerType, baseMethod,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    propertyMethodArg = new UaPropertyMethodArgument(
            UaNodeId(Ba_FurnaceControllerType_StartWithSetpoint_In,
            getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            1, // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetpoint
    propertyMethodArg->setArgument(0, // Index of the argument
            "TemperatureSetpoint", // Name of the argument
            UaNodeId(OpcUaId_Double), // Data type of the argument
            -1, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Add property to method
    addStatus = addNodeAndReference(baseMethod, propertyMethodArg,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the ControllerEventType and its event field properties
     **************************************************************/
    UaObjectTypeSimple* controllerEventType = new UaObjectTypeSimple("ControllerEventType", // Used as string in browse name and display name
            UaNodeId(Ba_ControllerEventType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,
            OpcUa_True);
    // Add Event Type node to address space as subtype of BaseEventType
    addStatus = addNodeAndReference(OpcUaId_BaseEventType, controllerEventType,
            OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    // ControllerConfigurations Event field property
    defaultValue.clear();
    UaPropertyCache *controllerConfigurations = new UaPropertyCache(
            "ControllerConfigurations", // used as BrowseName + DisplayName
            UaNodeId(Ba_ControllerEventType_ControllerConfigurations, getNameSpaceIndex()),
            defaultValue, Ua_AccessLevel_CurrentRead, m_defaultLocaleId);
    controllerConfigurations->setDataType(UaNodeId(Ba_Configurations, getNameSpaceIndex()));
    addStatus = addNodeAndReference(controllerEventType, controllerConfigurations,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    objectTypes.push_back(controllerEventType);

    /**************************************************************
     * Create the ValuesControllerEventType and its event field properties
     **************************************************************/
    UaObjectTypeSimple* valuesControllerEventType = new UaObjectTypeSimple("ValuesControllerEventType", // Used as string in browse name and display name
            UaNodeId(Ba_ValuesControllerEventType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,
            OpcUa_True);
    // Add Event Type node to address space as subtype of BaseEventType
    addStatus = addNodeAndReference(controllerEventType, valuesControllerEventType,
            OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    // Temperature Event field property
    defaultValue.setDouble(0);
    UaPropertyCache* temperature = new UaPropertyCache(
            "Temperature", // used as BrowseName + DisplayName
            UaNodeId(Ba_ValuesControllerEventType_Temperature, getNameSpaceIndex()),
            defaultValue, Ua_AccessLevel_CurrentRead, m_defaultLocaleId);
    addStatus = addNodeAndReference(valuesControllerEventType, temperature,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // State Event field property
    defaultValue.setUInt32(0);
    UaPropertyCache* state = new UaPropertyCache(
            "State", // used as BrowseName + DisplayName
            UaNodeId(Ba_ValuesControllerEventType_State, getNameSpaceIndex()),
            defaultValue, Ua_AccessLevel_CurrentRead, m_defaultLocaleId);
    addStatus = addNodeAndReference(valuesControllerEventType, state,
            OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Create reference "GeneratesEvent" from ControllerType to ValuesControllerEventType
    addStatus = addUaReference(UaNodeId(Ba_ControllerType, getNameSpaceIndex()),
            UaNodeId(Ba_ValuesControllerEventType, getNameSpaceIndex()),
            OpcUaId_GeneratesEvent);
    UA_ASSERT(addStatus.isGood());

    objectTypes.push_back(valuesControllerEventType);

    /**************************************************************
     * Register the event types and fields
     * to allow selection of custom event fields
     **************************************************************/
    EventTypeRegistry& eventTypeRegistry = getEventTypeRegistry();
    EventManagerUaNode::registerEventType(OpcUaId_BaseEventType,
            controllerEventType->nodeId());
    eventTypeRegistry.registerEventField(controllerEventType->nodeId(),
            controllerConfigurations->nodeId(), controllerConfigurations->browseName());
    EventManagerUaNode::registerEventType(controllerEventType->nodeId(),
            valuesControllerEventType->nodeId());
    eventTypeRegistry.registerEventField(valuesControllerEventType->nodeId(),
            temperature->nodeId(), temperature->browseName());
    eventTypeRegistry.registerEventField(valuesControllerEventType->nodeId(),
            state->nodeId(), state->browseName());

    getIODataProviderBridge().updateValueHandling(objectTypes);
    return ret;
}

UaStatus BaNodeManager::createObjectNodes() {
    std::vector<UaVariable*> variables;
    /**************************************************************
     Create a folder for the controller objects and add the folder to the ObjectsFolder
     ***************************************************************/
    UaFolder* buildingAutomation = new EventNotifierFolder("BuildingAutomation",
            UaNodeId("BuildingAutomation", getNameSpaceIndex()),
            m_defaultLocaleId);
    UaStatus ret = addNodeAndReference(OpcUaId_ObjectsFolder, buildingAutomation,
            OpcUaId_Organizes);

    // create an explicit reference from the server object (implicit event notifier for all 
    // event sources) to the new event notifier
    ret = addUaReference(UaNodeId(OpcUaId_Server), buildingAutomation->nodeId(),
            OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());

    //Configuration Variable
    UaVariant defaultConfiguration;
    UaString name("ControllerConfigurations");
    OpcUa::BaseDataVariableType *controllerConfigurations =
            new OpcUa::BaseDataVariableType(
            UaNodeId(name, getNameSpaceIndex()),
            name,
            getNameSpaceIndex(),
            defaultConfiguration,
            OpcUa_AccessLevels_CurrentRead | OpcUa_AccessLevels_CurrentWrite,
            this /*pNodeConfig*/,
            NULL /*pSharedMutex*/);
    controllerConfigurations->setDataType(UaNodeId(Ba_Configurations, getNameSpaceIndex()));
    controllerConfigurations->setValueRank(OpcUa_ValueRanks_Scalar);
    addNodeAndReference(buildingAutomation, controllerConfigurations, OpcUaId_Organizes);
    variables.push_back(controllerConfigurations);

    /**************************************************************
     * Create the Controller Object Instances
     * (the controller type has a reference to ValuesControllerEventType
     * => the controller objects are event sources)
     **************************************************************/
    OpcUa_UInt32 controllerAddress = 1;
    name = UaString("AirConditioner_1");
    AirConditionerControllerObject* airConditioner_1 =
            new AirConditionerControllerObject(name,
            UaNodeId(name, getNameSpaceIndex()),
            m_defaultLocaleId, *this, controllerAddress);
    ret = addNodeAndReference(buildingAutomation, airConditioner_1,
            OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // create a reference from an event notifier to the new event source
    ret = addUaReference(buildingAutomation->nodeId(), airConditioner_1->nodeId(),
            OpcUaId_HasEventSource);
    UA_ASSERT(ret.isGood());

    controllerAddress = 2;
    name = UaString("Furnace_2");
    FurnaceControllerObject* furnace_2 = new FurnaceControllerObject(
            name, UaNodeId(name, getNameSpaceIndex()),
            m_defaultLocaleId, *this, controllerAddress);
    ret = addNodeAndReference(buildingAutomation, furnace_2, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // create a reference from an event notifier to the new event source
    ret = addUaReference(buildingAutomation->nodeId(), furnace_2->nodeId(),
            OpcUaId_HasEventSource);
    UA_ASSERT(ret.isGood());

    getIODataProviderBridge().updateValueHandling(variables);
    return ret;
}
