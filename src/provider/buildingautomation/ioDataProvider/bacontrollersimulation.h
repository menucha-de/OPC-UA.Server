#ifndef PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BACONTROLLERSIMULATION_H
#define PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BACONTROLLERSIMULATION_H

#include "uabase.h"
#include "uamutex.h"
#include "uastring.h"
#include "bacommunicationinterface.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/Structure.h>

class BaControllerSimulation {
    UA_DISABLE_COPY(BaControllerSimulation);
public:
    BaControllerSimulation(OpcUa_UInt32 controllerAddress);
    virtual ~BaControllerSimulation();

    /* Get Controller status and data */
    virtual UaStatusCode getControllerState(
            BaCommunicationInterface::ControllerState& state) = 0;

    virtual UaStatusCode getControllerData(
            BaCommunicationInterface::Offset offset, OpcUa_Double& value) = 0;

    /* set Controller status and data */
    virtual void setControllerState(
            BaCommunicationInterface::ControllerState state,
            bool sendValueChangedEvents) = 0;

    virtual UaStatusCode setControllerData(
            BaCommunicationInterface::Offset offset, OpcUa_Double value,
            bool sendValueChangedEvents) = 0;

    virtual void subscribe(BaCommunicationInterface::Offset offset,
            const IODataProviderNamespace::NodeId& nodeId,
            IODataProviderNamespace::SubscriberCallback& callback);

    virtual void fireControllerEvent();

    virtual void simulate() = 0;

protected:
    CommonNamespace::Logger* log;
    UaMutex m_mutex;
    BaCommunicationInterface::ControllerState m_state;
    OpcUa_Double m_diff;
    OpcUa_Double m_temperatureSetpoint;
    OpcUa_Double m_temperature;
    OpcUa_Double m_temperatureDiff;
    OpcUa_Double m_powerConsumption;

    void fireEvent(BaCommunicationInterface::Offset offset,
            IODataProviderNamespace::Variant& value);
private:

    struct CallbackData {
        const IODataProviderNamespace::NodeId* nodeId;
        IODataProviderNamespace::SubscriberCallback* callback;
    };
    // offset -> callback data
    std::map<BaCommunicationInterface::Offset, CallbackData*> callbacks;
    OpcUa_UInt32 controllerAddress;

    IODataProviderNamespace::Structure* getControllerConfigurations() const;
};

class BaAirConditionerSimulation : public BaControllerSimulation {
    UA_DISABLE_COPY(BaAirConditionerSimulation);
public:
    /* construction / destruction */
    BaAirConditionerSimulation(OpcUa_UInt32 controllerAddress);
    virtual ~BaAirConditionerSimulation();

    /* Get Controller status and data */
    UaStatusCode getControllerState(
            BaCommunicationInterface::ControllerState& state);

    UaStatusCode getControllerData(BaCommunicationInterface::Offset offset,
            OpcUa_Double& value);

    /* set Controller status and data */
    void setControllerState(BaCommunicationInterface::ControllerState state,
            bool sendValueChangedEvents);

    UaStatusCode setControllerData(BaCommunicationInterface::Offset offset,
            OpcUa_Double value, bool sendValueChangedEvents);

    void simulate();
protected:
    OpcUa_Double m_humiditySetpoint;
    OpcUa_Double m_humidity;
    OpcUa_Double m_humidityDiff;
};

class BaFurnaceSimulation : public BaControllerSimulation {
    UA_DISABLE_COPY(BaFurnaceSimulation);
public:
    /* construction / destruction */
    BaFurnaceSimulation(OpcUa_UInt32 controllerAddress);
    virtual ~BaFurnaceSimulation();

    /* Get Controller status and data */
    UaStatusCode getControllerState(
            BaCommunicationInterface::ControllerState& state);

    UaStatusCode getControllerData(BaCommunicationInterface::Offset offset,
            OpcUa_Double& value);

    /* set Controller status and data */
    void setControllerState(BaCommunicationInterface::ControllerState state,
            bool sendValueChangedEvents);

    UaStatusCode setControllerData(BaCommunicationInterface::Offset offset,
            OpcUa_Double value, bool sendValueChangedEvents);

    void simulate();
protected:
    OpcUa_Double m_gasFlow;
};

#endif // PROVIDER_BUILDINGAUTOMATION_IODATAPROVIDER_BACONTROLLERSIMULATION_H
