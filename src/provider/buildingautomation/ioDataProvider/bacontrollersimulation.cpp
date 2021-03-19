#include "bacontrollersimulation.h"
#include "../buildingautomationtypeids.h"
#include <ioDataProvider/Array.h>
#include <ioDataProvider/OpcUaEventData.h>
#include <ioDataProvider/Scalar.h>
#include "uadatetime.h"

using namespace CommonNamespace;
using namespace IODataProviderNamespace;
// Scalar

BaControllerSimulation::BaControllerSimulation(OpcUa_UInt32 controllerAddress) {
    log = LoggerFactory::getLogger("BaControllerSimulation");
    this->controllerAddress = controllerAddress;
    m_diff = 1.86;
    m_state = BaCommunicationInterface::ON;
    m_temperatureSetpoint = 72;
    m_temperature = m_temperatureSetpoint + m_diff * 2;
    m_temperatureDiff = m_diff / 2;
    m_powerConsumption = 0;
}

BaControllerSimulation::~BaControllerSimulation() {
}

void BaControllerSimulation::subscribe(BaCommunicationInterface::Offset offset,
        const NodeId& nodeId, SubscriberCallback& callback) {
    //TODO delete callbacks in "unsubscribe"
    CallbackData* callbackData = new CallbackData();
    callbackData->nodeId = new NodeId(nodeId);
    callbackData->callback = &callback;
    callbacks[offset] = callbackData;
    if (nodeId.getNodeType() == NodeId::NUMERIC) {
        log->info("SUB %-20ld offset=%d", nodeId.getNumeric(), offset);
    } else {
        log->info("SUB %-20s offset=%d", nodeId.getString().c_str(), offset);
    }
}

void BaControllerSimulation::fireEvent(BaCommunicationInterface::Offset offset,
        Variant& value) {
    // get callback for offset
    std::map<BaCommunicationInterface::Offset, CallbackData*>::iterator callbackIter =
            callbacks.find(offset);
    if (callbackIter != callbacks.end()) {
        CallbackData& callbackData = *(*callbackIter).second;
        // create event
        std::vector<const NodeData*>* nodeData =
                new std::vector<const NodeData*>();
        nodeData->push_back(new NodeData(*callbackData.nodeId, &value));
        // send event via callback
        callbackData.callback->valuesChanged(
                Event(time(NULL) * 1000, *nodeData, true /* attachValues */));
    }
}

void BaControllerSimulation::fireControllerEvent() {
    int nsIndex = 2;
    UaString controllerName = UaString("AirConditioner_%1").arg(
            controllerAddress);

    const UaString& message = UaString("State of %1 changed to %2").arg(
            controllerName).arg(
            m_state == BaCommunicationInterface::OFF ? "OFF" : "ON");

    std::vector<const NodeData*>* fieldData =
            new std::vector<const NodeData*>();
    fieldData->push_back(
            new NodeData(*new NodeId(nsIndex, Ba_ControllerEventType_ControllerConfigurations),
            getControllerConfigurations(), true /* attachValue */));
    Scalar* s = new Scalar();
    s->setULong(m_state);
    fieldData->push_back(
            new NodeData(*new NodeId(nsIndex, Ba_ValuesControllerEventType_State),
            s, true /* attachValue */));
    s = new Scalar();
    s->setDouble(m_temperature);
    fieldData->push_back(
            new NodeData(*new NodeId(nsIndex, Ba_ValuesControllerEventType_Temperature),
            s, true /* attachValue */));
    OpcUaEventData eventData(*new NodeId(
            nsIndex, *new std::string(controllerName.toUtf8()), true /* attachValues */),
            *new std::string(message.toUtf8()), 500 /* severity */, *fieldData,
            true /* attachValues */);
    fireEvent(BaCommunicationInterface::VALUES_CONTROLLER_EVENT, eventData);
}

Structure* BaControllerSimulation::getControllerConfigurations() const {
    //TODO get real config
    int nsIndex = 2;
    std::map<std::string, const Variant*>* airConditionerController1Fields =
            new std::map<std::string, const Variant*>();
    Scalar* s = new Scalar();
    s->setString(new std::string("AirConditioner"), true /*attachValues*/);
    (*airConditionerController1Fields)[std::string("Name")] = s;
    s = new Scalar();
    s->setULong(1);
    (*airConditionerController1Fields)[std::string("DeviceAddress")] = s;
    s = new Scalar();
    s->setDouble(13);
    (*airConditionerController1Fields)[std::string("TemperatureSetpoint")] = s;
    s = new Scalar();
    s->setDouble(14);
    (*airConditionerController1Fields)[std::string("HumiditySetpoint")] = s;

    std::map<std::string, const Variant*>* airConditionerController2Fields =
            new std::map<std::string, const Variant*>();
    s = new Scalar();
    s->setString(new std::string("AirConditioner"), true /*attachValues*/);
    (*airConditionerController2Fields)[std::string("Name")] = s;
    s = new Scalar();
    s->setULong(11);
    (*airConditionerController2Fields)[std::string("DeviceAddress")] = s;
    s = new Scalar();
    s->setDouble(113);
    (*airConditionerController2Fields)[std::string("TemperatureSetpoint")] = s;
    s = new Scalar();
    s->setDouble(114);
    (*airConditionerController2Fields)[std::string("HumiditySetpoint")] = s;

    std::vector<const Variant*>* airConditionerControllers = new std::vector<const Variant*>();
    airConditionerControllers->push_back(
            new Structure(*new NodeId(nsIndex, Ba_AirConditionerControllerConfiguration),
            *airConditionerController1Fields, true /*attachValue*/));
    airConditionerControllers->push_back(
            new Structure(*new NodeId(nsIndex, Ba_AirConditionerControllerConfiguration),
            *airConditionerController2Fields, true /*attachValue*/));

    std::map<std::string, const Variant*>* furnaceController1Fields =
            new std::map<std::string, const Variant*>();
    s = new Scalar();
    s->setString(new std::string("Furnace"), true /*attachValues*/);
    (*furnaceController1Fields)[std::string("Name")] = s;
    s = new Scalar();
    s->setULong(2);
    (*furnaceController1Fields)[std::string("DeviceAddress")] = s;
    s = new Scalar();
    s->setDouble(23);
    (*furnaceController1Fields)[std::string("TemperatureSetpoint")] = s;

    std::vector<const Variant*>* furnaceControllers = new std::vector<const Variant*>();
    furnaceControllers->push_back(
            new Structure(*new NodeId(nsIndex, Ba_ControllerConfiguration),
            *furnaceController1Fields, true /*attachValue*/));

    std::map<std::string, const Variant*>* baConfigurationsFields =
            new std::map<std::string, const Variant*>();
    (*baConfigurationsFields)[std::string("AirConditionerControllers")] =
            new Array(Variant::STRUCTURE, airConditionerControllers, true /*attachValues*/);
    (*baConfigurationsFields)[std::string("FurnaceControllers")] =
            new Array(Variant::STRUCTURE, furnaceControllers, true /* attachValues */);
    return new Structure(*new NodeId(nsIndex, Ba_Configurations),
            *baConfigurationsFields, true /* attachValues */);
}

BaAirConditionerSimulation::BaAirConditionerSimulation(
        OpcUa_UInt32 controllerAddress) :
BaControllerSimulation(controllerAddress) {
    m_humiditySetpoint = 60;
    m_humidity = m_humiditySetpoint;
    m_humidityDiff = m_diff / 4;
}

BaAirConditionerSimulation::~BaAirConditionerSimulation() {
}

UaStatusCode BaAirConditionerSimulation::getControllerState(
        BaCommunicationInterface::ControllerState& state) {
    UaMutexLocker lock(&m_mutex);
    log->info("GET %-20s %d\n", "AirConditioner.State", m_state);
    state = m_state;
    return OpcUa_Good;
}

void BaAirConditionerSimulation::setControllerState(
        BaCommunicationInterface::ControllerState state,
        bool sendValueChangedEvents) {
    UaMutexLocker lock(&m_mutex);
    log->info("SET %-20s %d -> %d", "AirConditioner.State", m_state, state);
    if (state != m_state) {
        m_state = state;
        if (sendValueChangedEvents) {
            Scalar eventValue;
            eventValue.setULong(state);
            fireEvent(BaCommunicationInterface::STATE, eventValue);
        }
    }
}

UaStatusCode BaAirConditionerSimulation::getControllerData(
        BaCommunicationInterface::Offset offset, OpcUa_Double& value) {
    UaMutexLocker lock(&m_mutex);

    const char* id;
    switch (offset) {
        case BaCommunicationInterface::TEMPERATURE:
            value = m_temperature;
            id = "AirConditioner.Temperature";
            break;
        case BaCommunicationInterface::TEMPERATURE_SETPOINT:
            value = m_temperatureSetpoint;
            id = "AirConditioner.TemperatureSetpoint";
            break;
        case BaCommunicationInterface::POWER_CONSUMPTION:
            value = m_powerConsumption;
            id = "AirConditioner.PowerConsumption";
            break;
        case BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY:
            value = m_humidity;
            id = "AirConditioner.Humidity";
            break;
        case BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY_SETPOINT:
            value = m_humiditySetpoint;
            id = "AirConditioner.HumiditySetpoint";
            break;
        default:
            return OpcUa_BadInvalidArgument;
    }
    log->info("GET %-20s %f\n", id, value);
    return OpcUa_Good;
}

UaStatusCode BaAirConditionerSimulation::setControllerData(
        BaCommunicationInterface::Offset offset, OpcUa_Double value,
        bool sendValueChangedEvents) {
    UaMutexLocker lock(&m_mutex);
    UaStatusCode status;

    Scalar* eventValue = NULL;
    switch (offset) {
        case BaCommunicationInterface::TEMPERATURE:
            status = OpcUa_BadNotWritable;
            break;
        case BaCommunicationInterface::TEMPERATURE_SETPOINT:
            log->info("SET %-20s %f -> %f", "AirConditioner.TemperatureSetpoint",
                    m_temperatureSetpoint, value);
            if (value != m_temperatureSetpoint) {
                if (sendValueChangedEvents) {
                    eventValue = new Scalar();
                    eventValue->setDouble(value);
                }
                m_temperatureSetpoint = value;
            }
            break;
        case BaCommunicationInterface::POWER_CONSUMPTION:
            status = OpcUa_BadNotWritable;
            break;
        case BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY:
            status = OpcUa_BadNotWritable;
            break;
        case BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY_SETPOINT:
            log->info("SET %-20s %f -> %f", "AirConditioner.HumiditySetpoint", m_humiditySetpoint,
                    value);
            if (value != m_humiditySetpoint) {
                if (sendValueChangedEvents) {
                    eventValue = new Scalar();
                    eventValue->setDouble(value);
                }
                m_humiditySetpoint = value;
            }
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }
    if (eventValue != NULL) {
        fireEvent(offset, *eventValue);
        delete eventValue;
    }

    return status;
}

void BaAirConditionerSimulation::simulate() {
    UaMutexLocker lock(&m_mutex);
    double oldTemperature = m_temperature;
    double oldPowerConsumption = m_powerConsumption;
    double oldHumidity = m_humidity;
    if (m_state == BaCommunicationInterface::ON) {
        if (m_temperature > m_temperatureSetpoint * 1.05) {
            m_temperatureDiff = -m_diff;
        } else if (m_temperature < m_temperatureSetpoint * 0.9) {
            m_temperatureDiff = m_diff / 2;
        }
        if (m_humidity > m_humiditySetpoint * 1.05) {
            m_humidityDiff = -m_diff / 2;
        } else if (m_humidity < m_humiditySetpoint * 0.9) {
            m_humidityDiff = m_diff / 4;
        }
    } else {
        m_temperatureDiff = m_diff / 2;
        m_humidityDiff = m_diff / 4;
    }
    m_temperature += m_temperatureDiff;
    m_humidity += m_humidityDiff;
    if (m_temperatureDiff < 0 || m_humidityDiff < 0) {
        m_powerConsumption = m_diff * 200;
    } else {
        m_powerConsumption = 0;
    }
    if (oldTemperature != m_temperature) {
        Scalar eventValue;
        eventValue.setDouble(m_temperature);
        fireEvent(BaCommunicationInterface::TEMPERATURE, eventValue);
    }
    if (oldHumidity != m_humidity) {
        Scalar eventValue;
        eventValue.setDouble(m_humidity);
        fireEvent(BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY,
                eventValue);
    }
    if (oldPowerConsumption != m_powerConsumption) {
        Scalar eventValue;
        eventValue.setDouble(m_powerConsumption);
        fireEvent(BaCommunicationInterface::POWER_CONSUMPTION, eventValue);
    }
}

//----------------------------------------

BaFurnaceSimulation::BaFurnaceSimulation(OpcUa_UInt32 controllerAddress) :
BaControllerSimulation(controllerAddress) {
    m_gasFlow = 0;
    m_temperatureDiff = -m_diff / 2;
}

BaFurnaceSimulation::~BaFurnaceSimulation() {
}

/* ----------------------------------------------------------------------------
 Class        BaFurnaceSimulation
 Method       getControllerState
 Description  Get Controller status.
 -----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::getControllerState(
        BaCommunicationInterface::ControllerState& state) {
    UaMutexLocker lock(&m_mutex);
    log->info("GET %-20s %d", "Furnace.State", m_state);
    state = m_state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
 Class        BaFurnaceSimulation
 Method       setControllerState
 Description  Set Controller status.
 -----------------------------------------------------------------------------*/
void BaFurnaceSimulation::setControllerState(
        BaCommunicationInterface::ControllerState state,
        bool sendValueChangedEvents) {
    UaMutexLocker lock(&m_mutex);
    log->info("SET %-20s %d -> %d", "Furnace.State", m_state, state);
    if (state != m_state) {
        m_state = state;
        if (sendValueChangedEvents) {
            Scalar eventValue;
            eventValue.setULong(state);
            fireEvent(BaCommunicationInterface::STATE, eventValue);
        }
    }
}

/* ----------------------------------------------------------------------------
 Class        BaFurnaceSimulation
 Method       getControllerData
 Description  Get Controller data.
 -----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::getControllerData(
        BaCommunicationInterface::Offset offset, OpcUa_Double& value) {
    UaMutexLocker lock(&m_mutex);
    const char* id;
    switch (offset) {
        case BaCommunicationInterface::TEMPERATURE:
            value = m_temperature;
            id = "Furnace.Temperature";
            break;
        case BaCommunicationInterface::TEMPERATURE_SETPOINT:
            value = m_temperatureSetpoint;
            id = "Furnace.TemperatureSetpoint";
            break;
        case BaCommunicationInterface::POWER_CONSUMPTION:
            value = m_powerConsumption;
            id = "Furnace.PowerConsumption";
            break;
        case BaCommunicationInterface::FURNACE_GAS_FLOW:
            value = m_gasFlow;
            id = "Furnace.GasFlow";
            break;
        default:
            return OpcUa_BadInvalidArgument;
    }
    log->info("GET %-20s %f", id, value);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
 Class        BaFurnaceSimulation
 Method       setControllerData
 Description  Set Controller data.
 -----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::setControllerData(
        BaCommunicationInterface::Offset offset, OpcUa_Double value,
        bool sendValueChangedEvents) {
    UaMutexLocker lock(&m_mutex);
    UaStatusCode status;
    Scalar* eventValue = NULL;
    switch (offset) {
        case BaCommunicationInterface::TEMPERATURE:
            status = OpcUa_BadNotWritable;
            break;
        case BaCommunicationInterface::TEMPERATURE_SETPOINT:
            log->info("SET %-20s %f -> %f", "Furnace.TemperatureSetpoint", m_temperatureSetpoint,
                    value);
            if (value != m_temperatureSetpoint) {
                if (sendValueChangedEvents) {
                    eventValue = new Scalar();
                    eventValue->setDouble(value);
                }
                m_temperatureSetpoint = value;
            }
            break;
        case BaCommunicationInterface::POWER_CONSUMPTION:
            status = OpcUa_BadNotWritable;
            break;
        case BaCommunicationInterface::FURNACE_GAS_FLOW:
            status = OpcUa_BadNotWritable;
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }
    if (eventValue != NULL) {
        fireEvent(offset, *eventValue);
        delete eventValue;
    }

    return status;
}

/* ----------------------------------------------------------------------------
 Class        BaFurnaceSimulation
 Method       simulate
 Description  Simulate Controller data.
 -----------------------------------------------------------------------------*/
void BaFurnaceSimulation::simulate() {
    UaMutexLocker lock(&m_mutex);
    double oldTemperature = m_temperature;
    double oldPowerConsumption = m_powerConsumption;
    double oldGasFlow = m_gasFlow;
    if (m_state == BaCommunicationInterface::ON) {
        if (m_temperature > m_temperatureSetpoint * 1.05) {
            m_temperatureDiff = -m_diff / 2;
        } else if (m_temperature < m_temperatureSetpoint * 0.9) {
            m_temperatureDiff = m_diff;
        }
    } else {
        m_temperatureDiff = -m_diff / 2;
    }
    m_temperature += m_temperatureDiff;
    if (m_temperatureDiff > 0) {
        m_powerConsumption = m_diff * 400;
        m_gasFlow = m_diff * 100;
    } else {
        m_powerConsumption = 0;
        m_gasFlow = 0;
    }

    if (oldTemperature != m_temperature) {
        Scalar eventValue;
        eventValue.setDouble(m_temperature);
        fireEvent(BaCommunicationInterface::TEMPERATURE, eventValue);
    }
    if (oldPowerConsumption != m_powerConsumption) {
        Scalar eventValue;
        eventValue.setDouble(m_powerConsumption);
        fireEvent(BaCommunicationInterface::POWER_CONSUMPTION, eventValue);
    }
    if (oldGasFlow != m_gasFlow) {
        Scalar eventValue;
        eventValue.setDouble(m_gasFlow);
        fireEvent(BaCommunicationInterface::FURNACE_GAS_FLOW, eventValue);
    }
}

/* ----------------------------------------------------------------------------
 Begin Class    BaFurnaceSimulation
 constructors / destructors
 -----------------------------------------------------------------------------*/

