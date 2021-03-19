#include "controllerobject.h"
#include "../buildingautomationtypeids.h"
#include <ioDataProvider/NodeId.h>
#include <sasModelProvider/base/HaNodeManager.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include <nodemanagerbase.h> // NodeManagerBase
#include <opcua_analogitemtype.h> // OpcUa::AnalogItemType
#include <opcua_basedatavariabletype.h> // OpcUa::BaseDataVariableType
#include <opcua_dataitemtype.h> // OpcUa::DataItemType
#include <statuscode.h> // UaStatus
#include <uabasenodes.h> // UaVariable
#include <uamutex.h> // UaMutexRefCounted
#include <uanodeid.h> // UaNode
#include <uaobjecttypes.h> // UaObjectBase
#include <uaplatformdefs.h> // UA_ASSERT
#include <uastring.h> // UaString
#include <stddef.h> // NULL

using namespace SASModelProviderNamespace;

ControllerObject::ControllerObject(const UaString& name,
        const UaNodeId& newNodeId, const UaString& defaultLocaleId,
        HaNodeManager& haNodeManager, OpcUa_UInt32 deviceAddress) :
UaObjectBase(name, newNodeId, defaultLocaleId) {
    this->haNodeManager = &haNodeManager;
    nodeBrowser = new NodeBrowser(haNodeManager);
    NodeManagerBase* nodeManager = &haNodeManager.getNodeManagerBase();

    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    UaVariable* pVariableDeclaration = NULL;
    OpcUa::BaseDataVariableType* pDataVariable = NULL;
    OpcUa::DataItemType* pDataItem = NULL;
    OpcUa::AnalogItemType* pAnalogItem = NULL;
    UaStatus addStatus;
    // Method helpers
    UaPropertyMethodArgument* pPropertyArg = NULL;
    UaUInt32Array nullarray;
    OpcUa_Int16 nsIdx = nodeManager->getNameSpaceIndex();

    std::vector<UaVariable*> variables;

    /**************************************************************
     * Create the Controller components
     **************************************************************/
    // Add Variable "State"
    // Get the instance declaration node used as base for this variable instance
    pVariableDeclaration = getVariable(
            Ba_ControllerType_State);
    UA_ASSERT(pVariableDeclaration != NULL);
    pDataVariable = new OpcUa::BaseDataVariableType(this, // Parent node
            pVariableDeclaration, // Instance declaration variable this variable instance is based on
            nodeManager, // Node manager responsible for this variable
            m_pSharedMutex); // Shared mutex used across all variables of this object
    pVariableDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pDataVariable,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    variables.push_back(pDataVariable);

    // Add Variable "Temperature"
    pVariableDeclaration = getVariable(
            Ba_ControllerType_Temperature);
    UA_ASSERT(pVariableDeclaration != NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(this, pVariableDeclaration,
            nodeManager, m_pSharedMutex);
    pVariableDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pAnalogItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    pAnalogItem->setEURange(tempRange);
    UaEUInformation tempEUInformation(
            "http://www.opcfoundation.org/UA/units/un/cefact", 4604232,
            UaLocalizedText("en", "\xc2\xb0\x46") /* °F */,
            UaLocalizedText("en", "degree Fahrenheit"));
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    variables.push_back(pAnalogItem);

    // Add Variable "TemperatureSetpoint"
    // Get the instance declaration node used as base for this variable instance
    pVariableDeclaration = getVariable(
            Ba_ControllerType_TemperatureSetpoint);
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

    // Add Variable "PowerConsumption"
    // Get the instance declaration node used as base for this variable instance
    pVariableDeclaration = getVariable(
            Ba_ControllerType_PowerConsumption);
    UA_ASSERT(pVariableDeclaration != NULL);
    // Create new variable and add it as component to this object
    pDataItem = new OpcUa::DataItemType(this, pVariableDeclaration, nodeManager,
            m_pSharedMutex);
    pVariableDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pDataItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    variables.push_back(pDataItem);

    // Add Method "Start"
    UaMethod* pMethodDeclaration = getMethod(
            Ba_ControllerType_Start);
    OpcUa::BaseMethod* pMethodStart = new OpcUa::BaseMethod(this,
            pMethodDeclaration, m_pSharedMutex);
    pMethodDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pMethodStart,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethodDeclaration = getMethod(
            Ba_ControllerType_Stop);
    OpcUa::BaseMethod* pMethodStop = new OpcUa::BaseMethod(this,
            pMethodDeclaration, m_pSharedMutex);
    pMethodDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pMethodStop,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "SetControllerConfigurations"
    pMethodDeclaration = getMethod(Ba_ControllerType_SetControllerConfigurations);
    OpcUa::BaseMethod* pMethodSetControllerConfigurations = new OpcUa::BaseMethod(this,
            pMethodDeclaration, m_pSharedMutex);
    pMethodDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pMethodSetControllerConfigurations,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    UaString sNodeId = UaString("%1.InputArguments").arg(
            pMethodSetControllerConfigurations->nodeId().toString());
    pPropertyArg = new UaPropertyMethodArgument(UaNodeId(sNodeId, nsIdx), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            1, // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument ControllerConfigurations
    pPropertyArg->setArgument(0, // Index of the argument
            "ControllerConfigurations", // Name of the argument
            UaNodeId(Ba_Configurations, nsIdx), // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller configurations")); // Description
    // Add property to method
    addStatus = nodeManager->addNodeAndReference(pMethodSetControllerConfigurations,
            pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
    // Add Property "OutputArguments"
    sNodeId = UaString("%1.OutputArguments").arg(
            pMethodSetControllerConfigurations->nodeId().toString());
    pPropertyArg = new UaPropertyMethodArgument(UaNodeId(sNodeId, nsIdx), // NodeId of the property
            Ua_AccessLevel_CurrentRead, // Access level of the property
            1, // Number of arguments
            UaPropertyMethodArgument::OUTARGUMENTS);
    // Argument ControllerConfigurations
    pPropertyArg->setArgument(0, // Index of the argument
            "ControllerConfigurations", // Name of the argument
            UaNodeId(Ba_Configurations, nsIdx), // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller configurations")); // Description
    // Add property to method
    addStatus = nodeManager->addNodeAndReference(pMethodSetControllerConfigurations,
            pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    haNodeManager.getIODataProviderBridge().updateValueHandling(variables);
}

ControllerObject::~ControllerObject(void) {
    delete nodeBrowser;
    if (m_pSharedMutex) {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

OpcUa_Byte ControllerObject::eventNotifier() const {
    return Ua_EventNotifier_None;
}

MethodManager* ControllerObject::getMethodManager(UaMethod* pMethod) const {
    // the node manager is the method manager for all registered objects
    return (MethodManager*) haNodeManager;
}

UaVariable* ControllerObject::getVariable(OpcUa_UInt32 nodeId) {
    UaNodeId nid(nodeId, haNodeManager->getNodeManagerBase().getNameSpaceIndex());
    return nodeBrowser->getVariable(nid);
}

UaMethod* ControllerObject::getMethod(OpcUa_UInt32 nodeId) {
    UaNodeId nid(nodeId, haNodeManager->getNodeManagerBase().getNameSpaceIndex());
    return nodeBrowser->getMethod(nid);
}
