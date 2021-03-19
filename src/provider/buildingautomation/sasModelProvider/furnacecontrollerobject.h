#ifndef PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_FURNACECONTROLLEROBJECT_H
#define PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_FURNACECONTROLLEROBJECT_H

#include "controllerobject.h" // ControllerObject
#include <sasModelProvider/base/HaNodeManager.h>
#include "opcua_baseobjecttype.h" // OpcUa::BaseMethod
#include "uastring.h" // UaString
#include "uanodeid.h" // UaNodeId

class FurnaceControllerObject: public ControllerObject {
public:
	FurnaceControllerObject(const UaString& name, const UaNodeId& newNodeId,
			const UaString& defaultLocaleId,
			SASModelProviderNamespace::HaNodeManager& haNodeManager,
			OpcUa_UInt32 deviceAddress);
	virtual ~FurnaceControllerObject(void);

	virtual UaNodeId typeDefinitionId() const;
private:
	OpcUa::BaseMethod *m_pMethodStartWithSetpoint;
};

#endif // PROVIDER_BUILDINGAUTOMATION_SASMODELPROVIDER_FURNACECONTROLLEROBJECT_H
