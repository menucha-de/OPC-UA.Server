#include "airconditionercontrollerobject.h"
#include "../buildingautomationtypeids.h"
#include <ioDataProvider/NodeId.h>
#include <sasModelProvider/base/HaNodeManager.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include "controllerobject.h" // ControllerObject
#include <nodemanagerbase.h> // NodeManagerBase
#include <opcua_analogitemtype.h> // OpcUa::AnalogItemType
#include "statuscode.h" // UaStatus
#include "uaarraytemplates.h" // UaUInt32Array
#include "uabasenodes.h" // UaVariable
#include "uadatavariablecache.h" // UaPropertyMethodArgument
#include "uaeuinformation.h" // UaEUInformation
#include "uanodeid.h" // UaNodeId
#include "uaplatformdefs.h" // UA_ASSERT
#include "uastring.h" // UaString
#include <vector>

using namespace SASModelProviderNamespace;

AirConditionerControllerObject::AirConditionerControllerObject(
        const UaString& name, const UaNodeId& newNodeId,
        const UaString& defaultLocaleId, HaNodeManager& haNodeManager,
        OpcUa_UInt32 deviceAddress) :
ControllerObject(name, newNodeId, defaultLocaleId, haNodeManager,
deviceAddress) {
    NodeManagerBase* nodeManager = &haNodeManager.getNodeManagerBase();

    UaVariable* pVariableDeclaration = NULL;
    OpcUa::AnalogItemType* pAnalogItem = NULL;
    UaStatus addStatus;
    // Method helpers
    UaPropertyMethodArgument* pPropertyArg = NULL;
    UaUInt32Array nullarray;
    OpcUa_Int16 nsIdx = nodeManager->getNameSpaceIndex();

    std::vector<UaVariable*> variables;

    /**************************************************************
     * Create the AirConditionerController components
     **************************************************************/
    // Add Variable "Humidity"
    pVariableDeclaration = getVariable(
            Ba_AirConditionerControllerType_Humidity);
    UA_ASSERT(pVariableDeclaration != NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(this, // Parent node
            pVariableDeclaration, // Instance declaration variable this variable instance is based on
            nodeManager, // Node manager responsible for this variable
            m_pSharedMutex); // Shared mutex used across all variables of this object
    pVariableDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pAnalogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    pAnalogItem->setEURange(tempRange);
    UaEUInformation tempEUInformation("", -1, UaLocalizedText("en", "%"),
            UaLocalizedText("en", "Percent"));
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    variables.push_back(pAnalogItem);

    // Add Variable "HumiditySetpoint"
    // Get the instance declaration node used as base for this variable instance
    pVariableDeclaration = getVariable(
            Ba_AirConditionerControllerType_HumiditySetpoint);
    UA_ASSERT(pVariableDeclaration != NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(this, pVariableDeclaration,
            nodeManager, m_pSharedMutex);
    pVariableDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pAnalogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    variables.push_back(pAnalogItem);

    // Add Method "StartWithSetpoint"
    UaMethod* pMethodDeclaration = getMethod(
            Ba_AirConditionerControllerType_StartWithSetpoint);
    m_pMethodStartWithSetpoint = new OpcUa::BaseMethod(this, pMethodDeclaration,
            m_pSharedMutex);
    pMethodDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this,
            m_pMethodStartWithSetpoint, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    UaString sNodeId = UaString("%1.StartWithSetpoint").arg(
            m_pMethodStartWithSetpoint->nodeId().toString());
    pPropertyArg = new UaPropertyMethodArgument(UaNodeId(sNodeId, nsIdx), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            2, // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetpoint
    pPropertyArg->setArgument(0, // Index of the argument
            "TemperatureSetpoint", // Name of the argument
            UaNodeId(OpcUaId_Double), // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Argument HumiditySetpoint
    pPropertyArg->setArgument(1, // Index of the argument
            "HumiditySetpoint", // Name of the argument
            UaNodeId(OpcUaId_Double), // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller set point for humidity")); // Description
    // Add property to method
    addStatus = nodeManager->addNodeAndReference(m_pMethodStartWithSetpoint,
            pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    haNodeManager.getIODataProviderBridge().updateValueHandling(variables);
}

AirConditionerControllerObject::~AirConditionerControllerObject(void) {
}

UaNodeId AirConditionerControllerObject::typeDefinitionId() const {
    UaNodeId ret(Ba_AirConditionerControllerType,
            browseName().namespaceIndex());
    return ret;
}

