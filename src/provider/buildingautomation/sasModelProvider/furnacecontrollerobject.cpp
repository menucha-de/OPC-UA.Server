#include "furnacecontrollerobject.h"

#include "../buildingautomationtypeids.h"
#include <sasModelProvider/base/HaNodeManager.h>
#include <sasModelProvider/base/HaNodeManagerIODataProviderBridge.h>
#include "controllerobject.h" // ControllerObject
#include <nodemanagerbase.h> // NodeManagerBase
#include "opcua_dataitemtype.h" // OpcUa::DataItemType
#include "statuscode.h" // UaStatus
#include "uaarraytemplates.h" // UaUInt32Array
#include "uabasenodes.h" // UaVariable
#include "uadatavariablecache.h" // UaPropertyMethodArgument
#include "uanodeid.h"
#include "uaplatformdefs.h" // UA_ASSERT
#include "uastring.h"
#include <vector>

using namespace SASModelProviderNamespace;

FurnaceControllerObject::FurnaceControllerObject(const UaString& name,
        const UaNodeId& newNodeId, const UaString& defaultLocaleId,
        HaNodeManager& haNodeManager, OpcUa_UInt32 deviceAddress) :
ControllerObject(name, newNodeId, defaultLocaleId, haNodeManager,
deviceAddress) {
    NodeManagerBase* nodeManager = &haNodeManager.getNodeManagerBase();
    HaNodeManagerIODataProviderBridge& nmioBridge =
            haNodeManager.getIODataProviderBridge();

    UaVariable* pVariableDeclaration = NULL;
    OpcUa::DataItemType* pDataItem = NULL;
    UaStatus addStatus;
    // Method helpers
    UaPropertyMethodArgument* pPropertyArg = NULL;
    UaUInt32Array nullarray;
    OpcUa_Int16 nsIdx = nodeManager->getNameSpaceIndex();

    std::vector<UaVariable*> variables;

    /**************************************************************
     * Create the FurnaceController components
     **************************************************************/
    // Add Variable "GasFlow"
    // Get the instance declaration node used as base for this variable instance
    pVariableDeclaration = getVariable(
            Ba_FurnaceControllerType_GasFlow);
    UA_ASSERT(pVariableDeclaration != NULL);
    pDataItem = new OpcUa::DataItemType(this, // Parent node
            pVariableDeclaration, // Instance declaration variable this variable instance is based on
            nodeManager, // Node manager responsible for this variable
            m_pSharedMutex); // Shared mutex used across all variables of this object
    pVariableDeclaration->releaseReference();
    addStatus = nodeManager->addNodeAndReference(this, pDataItem,
            OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    variables.push_back(pDataItem);

    // Add Method "StartWithSetpoint"
    UaMethod* pMethodDeclaration = getMethod(
            Ba_FurnaceControllerType_StartWithSetpoint);
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
            1, // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetpoint
    pPropertyArg->setArgument(0, // Index of the argument
            "TemperatureSetpoint", // Name of the argument
            UaNodeId(OpcUaId_Double), // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray, // Array dimensions of the argument
            UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Add property to method
    addStatus = nodeManager->addNodeAndReference(m_pMethodStartWithSetpoint,
            pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    nmioBridge.updateValueHandling(variables);
}

FurnaceControllerObject::~FurnaceControllerObject(void) {
}

UaNodeId FurnaceControllerObject::typeDefinitionId() const {
    UaNodeId ret(Ba_FurnaceControllerType, browseName().namespaceIndex());
    return ret;
}
