#ifndef PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BACOMMUNICATIONINTERFACE_H
#define PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BACOMMUNICATIONINTERFACE_H

#include <ioDataProvider/SubscriberCallback.h>
#include "uabase.h"
#include "uastring.h"
#include "statuscode.h"
#include "uaarraytemplates.h"
#include "uathread.h"

class BaControllerSimulation;

class BaCommunicationInterface : public UaThread {
    UA_DISABLE_COPY(BaCommunicationInterface);
public:

    enum ControllerType {
        AIR_CONDITIONER, FURNACE
    };

    enum ControllerState {
        OFF = 0, ON = 1, ERROR = 2
    };

    enum Offset {
        // variables
        STATE,
        TEMPERATURE,
        TEMPERATURE_SETPOINT,
        POWER_CONSUMPTION,
        AIR_CONDITIONER_HUMIDITY,
        AIR_CONDITIONER_HUMIDITY_SETPOINT,
        FURNACE_GAS_FLOW,
        // methods
        START,
        START_WITH_SETPOINT,
        STOP,
        SET_CONTROLLER_CONFIGURATIONS,
        // events
        VALUES_CONTROLLER_EVENT
    };

    /* construction / destruction */
    BaCommunicationInterface();
    virtual ~BaCommunicationInterface();

    /* Get available controllers and their configuration */
    OpcUa_UInt32 getCountControllers() const;

    UaStatusCode getControllerConfig(OpcUa_UInt32 controllerIndex,
            ControllerType& type, UaString& sName, OpcUa_UInt32& address) const;

    /* Get Controller status and data */
    UaStatusCode getControllerState(OpcUa_UInt32 address,
            ControllerState& state);

    UaStatusCode getControllerData(OpcUa_UInt32 address,
            BaCommunicationInterface::Offset offset, OpcUa_Double& value);

    /* Set Controller status and data */
    UaStatusCode setControllerState(OpcUa_UInt32 address, ControllerState state,
            bool sendValueChangedEvents);

    UaStatusCode setControllerData(OpcUa_UInt32 address,
            BaCommunicationInterface::Offset offset, OpcUa_Double value,
            bool sendValueChangedEvents);

    UaStatusCode subscribe(OpcUa_UInt32 address,
            BaCommunicationInterface::Offset offset,
            const IODataProviderNamespace::NodeId& nodeId,
            IODataProviderNamespace::SubscriberCallback& callback);

    UaStatusCode fireControllerEvent(OpcUa_UInt32 address);

private:
    // Simulation Thread main function
    void run();

    UaObjectPointerArray<BaControllerSimulation> m_arrayDevices;
    OpcUa_Boolean m_stop;
};

#endif // PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BACOMMUNICATIONINTERFACE_H
