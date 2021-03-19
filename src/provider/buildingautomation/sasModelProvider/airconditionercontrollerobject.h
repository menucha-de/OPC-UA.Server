#ifndef PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_AIRCONDITIONERCONTROLLEROBJECT_H
#define PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_AIRCONDITIONERCONTROLLEROBJECT_H

#include "controllerobject.h" // ControllerObject
#include "opcua_baseobjecttype.h" // OpcUa::BaseMethod

class AirConditionerControllerObject: public ControllerObject {
public:
	AirConditionerControllerObject(const UaString& name,
			const UaNodeId& newNodeId, const UaString& defaultLocaleId,
			SASModelProviderNamespace::HaNodeManager& haNodeManager,
			OpcUa_UInt32 deviceAddress);
	virtual ~AirConditionerControllerObject(void);

	virtual UaNodeId typeDefinitionId() const;
private:
	OpcUa::BaseMethod *m_pMethodStartWithSetpoint;
};

#endif // PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_AIRCONDITIONERCONTROLLEROBJECT_H
