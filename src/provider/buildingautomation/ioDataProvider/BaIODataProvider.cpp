#include "BaIODataProvider.h"
#include "bacommunicationinterface.h" // BaCommunicationInterface
#include "../buildingautomationtypeids.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/Array.h>
#include <ioDataProvider/OpcUaEventData.h>
#include <ioDataProvider/Scalar.h>
#include <ioDataProvider/Structure.h>
#include <uastring.h> // UaString
#include "uaunistring.h" // UaUniString
#include <sstream>
#include <iostream>

using namespace CommonNamespace;
using namespace IODataProviderNamespace;

class BaIODataProviderPrivate {
    friend class BaIODataProvider;
private:
    Logger* log;
    BaCommunicationInterface baCommIf;

    // Parses a NodeId with a string identifier.
    void parseNodeId(const NodeId& nodeId,
            BaCommunicationInterface::ControllerType* returnControllerType,
            OpcUa_UInt32* returnControllerAddress,
            BaCommunicationInterface::Offset* returnOffset);

    Structure* getControllerConfigurations() const;
    void setControllerConfigurations(const Structure& controllerConfigurations);
};

BaIODataProvider::BaIODataProvider() {
    d = new BaIODataProviderPrivate();
    d->log = LoggerFactory::getLogger("BaIODataProvider");
}

BaIODataProvider::~BaIODataProvider() {
    delete d;
}

void BaIODataProvider::open(const std::string& confDir) {
}

void BaIODataProvider::close() {
}

const NodeProperties* BaIODataProvider::getDefaultNodeProperties(
        const std::string& namespaceUri, int namespaceId) {
    return new NodeProperties(NodeProperties::ASYNC);
}

std::vector<const NodeData*>* BaIODataProvider::getNodeProperties(
        const std::string& namespaceUri, int namespaceIndex) {
    std::vector<const NodeData*>* ret = new std::vector<const NodeData*>();
    int idsNoneCount = 12;
    const char* idsNone[idsNoneCount] = {
        "AirConditioner_1.Humidity.EURange",
        "AirConditioner_1.Humidity.EngineeringUnits",
        "AirConditioner_1.HumiditySetpoint.EURange",
        "AirConditioner_1.HumiditySetpoint.EngineeringUnits",
        "AirConditioner_1.Temperature.EURange",
        "AirConditioner_1.Temperature.EngineeringUnits",
        "AirConditioner_1.TemperatureSetpoint.EURange",
        "AirConditioner_1.TemperatureSetpoint.EngineeringUnits",
        "Furnace_2.Temperature.EURange",
        "Furnace_2.Temperature.EngineeringUnits",
        "Furnace_2.TemperatureSetpoint.EURange",
        "Furnace_2.TemperatureSetpoint.EngineeringUnits"
    };
    for (int i = 0; i < idsNoneCount; i++) {
        ret->push_back(
                new NodeData(
                *new NodeId(namespaceIndex,
                *new std::string(idsNone[i]),
                true /* attachValues */),
                new NodeProperties(NodeProperties::NONE),
                true /* attachValues */));
    }
    int idsNumNoneCount = 10;
    int idsNumNone[idsNumNoneCount] = {
        6001, // binary dictionary
        6002, //   NamespaceUri
        6005, //   ControllerConfiguration
        6007, //   AirConditionerControllerConfiguration
        6009, //   BAConfigurations

        6003, // xml dictionary
        6004, //   NamespaceUri
        6006, //   ControllerConfiguration
        6008, //   AirConditionerControllerConfiguration
        6011 //    BAConfigurations
    };
    for (int i = 0; i < idsNumNoneCount; i++) {
        ret->push_back(
                new NodeData(
                *new NodeId(namespaceIndex,
                idsNumNone[i]),
                new NodeProperties(NodeProperties::NONE),
                true /* attachValues */));
    }

    int idsSyncCount = 2;
    const char* idsSync[idsSyncCount] = {
        "AirConditioner_1.Temperature",
        "ControllerConfigurations"
    };
    for (int i = 0; i < idsSyncCount; i++) {
        ret->push_back(
                new NodeData(
                *new NodeId(namespaceIndex,
                *new std::string(idsSync[i]),
                true /* attachValues */),
                new NodeProperties(NodeProperties::SYNC),
                true /* attachValues */));
    }
    return ret;
}

std::vector<NodeData*>* BaIODataProvider::read(
        const std::vector<const NodeId*>& nodeIds) {
    std::vector<NodeData*>* ret = new std::vector<NodeData*>();
    for (std::vector<const NodeId*>::const_iterator i = nodeIds.begin();
            i != nodeIds.end(); i++) {
        const NodeId& nodeId = **i;

        BaCommunicationInterface::ControllerType controllerType;
        OpcUa_UInt32 controllerAddress;
        BaCommunicationInterface::Offset offset;
        if (nodeId.getString() == std::string("ControllerConfigurations")) {
            ret->push_back(new NodeData(*new NodeId(nodeId), d->getControllerConfigurations(),
                    true /* attachValues */));
            continue;
        }
        d->parseNodeId(nodeId, &controllerType, &controllerAddress, &offset);

        Scalar* s = new Scalar();
        // if the state shall be read
        if (offset == BaCommunicationInterface::STATE) {
            BaCommunicationInterface::ControllerState state;
            UaStatusCode status = d->baCommIf.getControllerState(controllerAddress,
                    state);
            if (!status.isGood()) {
                //TODO throw exception
            }
            s->setULong(state);
        } else {
            OpcUa_Double value;
            UaStatusCode status = d->baCommIf.getControllerData(controllerAddress,
                    offset, value);
            if (!status.isGood()) {
                //TODO throw exception
            }
            s->setDouble(value);
        }
        ret->push_back(new NodeData(*new NodeId(nodeId), s, true /* attachValues */));
    }
    return ret;
}

void BaIODataProvider::write(const std::vector<const NodeData*>& nodeData,
        bool sendValueChangedEvents) {
    for (std::vector<const NodeData*>::const_iterator i = nodeData.begin();
            i != nodeData.end(); i++) {
        const NodeData* nodeDataItem = *i;
        const NodeId& nodeId = nodeDataItem->getNodeId();

        BaCommunicationInterface::ControllerType controllerType;
        OpcUa_UInt32 controllerAddress;
        BaCommunicationInterface::Offset offset;
        if (nodeId.getString() == std::string("ControllerConfigurations")) {
            d->setControllerConfigurations(*static_cast<const Structure*> (nodeDataItem->getData()));
            continue;
        }
        d->parseNodeId(nodeId, &controllerType, &controllerAddress, &offset);

        UaStatusCode ret;
        if (offset == BaCommunicationInterface::STATE) {
            BaCommunicationInterface::ControllerState state;
            switch (static_cast<const Scalar*> (nodeDataItem->getData())->getULong()) {
                case 0:
                    state = BaCommunicationInterface::OFF;
                    break;
                case 1:
                    state = BaCommunicationInterface::ON;
                    break;
                case 2:
                    state = BaCommunicationInterface::ERROR;
                    break;
                default:
                    //TODO throw exception
                    state = BaCommunicationInterface::OFF;
            }
            ret = d->baCommIf.setControllerState(controllerAddress, state,
                    sendValueChangedEvents);
        } else {
            const Scalar& value = *static_cast<const Scalar*> (nodeDataItem->getData());
            ret = d->baCommIf.setControllerData(controllerAddress, offset,
                    value.getDouble(), sendValueChangedEvents);
        }
        if (!ret.isGood()) {
            //TODO throw exception
        }
    }
}

std::vector<MethodData*>* BaIODataProvider::call(
        const std::vector<const MethodData*>& methodData) {
    std::vector<MethodData*>* ret = new std::vector<MethodData*>();
    // for each method data item
    for (std::vector<const MethodData*>::const_iterator i = methodData.begin();
            i != methodData.end(); i++) {
        const MethodData& methodDataItem = **i;
        const NodeId& nodeId = methodDataItem.getMethodNodeId();

        BaCommunicationInterface::ControllerType controllerType;
        OpcUa_UInt32 controllerAddress;
        BaCommunicationInterface::Offset offset;
        d->parseNodeId(nodeId, &controllerType, &controllerAddress, &offset);

        UaStatusCode status = OpcUa_Good;
        const std::vector<const Variant*>* inputArgs = &methodDataItem.getMethodArguments();
        std::vector<const Variant*>* outputArgs = new std::vector<const Variant*>();
        switch (offset) {
            case BaCommunicationInterface::START:
                status = d->baCommIf.setControllerState(controllerAddress,
                        BaCommunicationInterface::ON,
                        true /* sendValueChangedEvents */);
                break;
            case BaCommunicationInterface::STOP:
                status = d->baCommIf.setControllerState(controllerAddress,
                        BaCommunicationInterface::OFF,
                        true /* sendValueChangedEvents */);
                break;
            case BaCommunicationInterface::START_WITH_SETPOINT:
            {
                // validate input arguments                
                const Scalar* humiditySetpoint = NULL;
                if (controllerType == BaCommunicationInterface::AIR_CONDITIONER) {
                    if (inputArgs->size() != 2) {
                        // TODO throw exception (OpcUa_BadInvalidArgument)
                    }
                    humiditySetpoint = static_cast<const Scalar*> (inputArgs->at(1));
                    if (humiditySetpoint->getScalarType() != Scalar::DOUBLE) {
                        // TODO throw exception (OpcUa_BadTypeMismatch)
                    }
                } else if (inputArgs->size() != 1) {
                    // TODO throw exception (OpcUa_BadInvalidArgument)
                }
                const Scalar* temperatureSetpoint = static_cast<const Scalar*> (inputArgs->at(0));
                if (temperatureSetpoint->getScalarType() != Scalar::DOUBLE) {
                    // TODO throw exception (OpcUa_BadTypeMismatch)
                }

                status = d->baCommIf.setControllerState(controllerAddress,
                        BaCommunicationInterface::ON,
                        true /* sendValueChangedEvents */);
                if (!status.isGood()) {
                    //TODO throw exception
                }
                status = d->baCommIf.setControllerData(controllerAddress,
                        BaCommunicationInterface::TEMPERATURE_SETPOINT,
                        temperatureSetpoint->getDouble(),
                        true /* sendValuesChangedEvents */);
                if (!status.isGood()) {
                    //TODO throw exception
                }
                if (humiditySetpoint != NULL) {
                    status =
                            d->baCommIf.setControllerData(controllerAddress,
                            BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY_SETPOINT,
                            humiditySetpoint->getDouble(),
                            true /* sendValuesChangedEvents */);
                }
                break;
            }
            case BaCommunicationInterface::SET_CONTROLLER_CONFIGURATIONS:
            {
                if (inputArgs->size() != 1) {
                    // TODO throw exception (OpcUa_BadInvalidArgument)
                }
                Structure& controllerConfigurations = *(Structure*) inputArgs->at(0);
                d->setControllerConfigurations(controllerConfigurations);
                // return the current controller configurations
                outputArgs->push_back(d->getControllerConfigurations());
                break;
            }
            default:
                // TODO throw exception (OpcUa_BadMethodInvalid)
                d->log->error("Cannot call unknown method with nodeId %s",
                        nodeId.toString().c_str());
                break;
        }
        if (!status.isGood()) {
            //TODO throw exception
        }

        d->baCommIf.fireControllerEvent(controllerAddress);

        MethodData* md = new MethodData(
                *new NodeId(methodDataItem.getObjectNodeId()),
                *new NodeId(methodDataItem.getMethodNodeId()), *outputArgs,
                true /* attachValues */);
        ret->push_back(md);
    }
    return ret;
}

std::vector<NodeData*>* BaIODataProvider::subscribe(const std::vector<const NodeId*>& nodeIds,
        SubscriberCallback& callback) {
    std::vector<const NodeId*> nodeIds4read;
    // for each nodeId
    for (std::vector<const NodeId*>::const_iterator i = nodeIds.begin();
            i != nodeIds.end(); i++) {
        const NodeId& nodeId = **i;
        if (nodeId.getNodeType() == NodeId::NUMERIC) {
            switch (nodeId.getNumeric()) {
                    //case Ba_ControllerEventType:
                case Ba_ValuesControllerEventType:
                {
                    BaCommunicationInterface::ControllerType controllerType;
                    UaString controllerName;
                    OpcUa_UInt32 controllerAddress;
                    // for each controller
                    for (OpcUa_UInt32 i = 0; i < d->baCommIf.getCountControllers(); i++) {
                        // get controller type + address + variable offset
                        UaStatusCode status = d->baCommIf.getControllerConfig(i,
                                controllerType, controllerName, controllerAddress);
                        if (!status.isGood()) {
                            //TODO throw exception
                        }
                        // subscribe
                        status = d->baCommIf.subscribe(controllerAddress,
                                BaCommunicationInterface::VALUES_CONTROLLER_EVENT /*offset*/,
                                nodeId, callback);
                        if (!status.isGood()) {
                            //TODO throw exception
                        }
                    }
                    break;
                }
                default:
                    d->log->error("Cannot subscribe to unknown nodeId %s",
                            nodeId.toString().c_str());
                    break;
            };
        } else if (nodeId.getNodeType() == NodeId::STRING
                && nodeId.getString() == std::string("ControllerConfigurations")) {
            nodeIds4read.push_back(&nodeId);
        } else {
            BaCommunicationInterface::ControllerType controllerType;
            OpcUa_UInt32 controllerAddress;
            BaCommunicationInterface::Offset offset;
            // get controller type + address + variable offset
            d->parseNodeId(nodeId, &controllerType, &controllerAddress, &offset);
            UaStatusCode status = d->baCommIf.subscribe(controllerAddress, offset,
                    nodeId, callback);
            if (!status.isGood()) {
                //TODO throw exception
            }
            nodeIds4read.push_back(&nodeId);
        }
    }
    return read(nodeIds4read);
}

void BaIODataProvider::unsubscribe(const std::vector<const NodeId*>& nodeIds) {
    //TODO
}

void BaIODataProviderPrivate::parseNodeId(const NodeId& nodeId,
        BaCommunicationInterface::ControllerType* returnControllerType,
        OpcUa_UInt32* returnControllerAddress,
        BaCommunicationInterface::Offset* returnOffset) {
    // AirConditioner_1.Start
    // => controllerTypeStr:    AirConditioner
    //    controllerAddressStr: 1
    //    offsetStr:            Start
    UaString id(nodeId.getString().c_str());
    UaUniString idStr(id.toUtf16());
    int idx1 = idStr.indexOf('_');
    UaUniString controllerTypeStr = idStr.left(idx1);
    int idx2 = idStr.indexOf('.', idx1);
    UaUniString controllerAddressStr = idStr.mid(idx1 + 1, idx2 - 1 - idx1);
    UaUniString offsetStr = idStr.right(idStr.length() - idx2 - 1);

    if (controllerTypeStr.startsWith("A")) {
        *returnControllerType = BaCommunicationInterface::AIR_CONDITIONER;
    } else {
        *returnControllerType = BaCommunicationInterface::FURNACE;
    }

    std::istringstream stream(
            UaString(controllerAddressStr.toUtf16()).toUtf8());
    stream >> *returnControllerAddress;

    *returnOffset = BaCommunicationInterface::STATE;
    if (offsetStr.startsWith("TemperatureS")) {
        *returnOffset = BaCommunicationInterface::TEMPERATURE_SETPOINT;
    } else if (offsetStr.startsWith("T")) {
        *returnOffset = BaCommunicationInterface::TEMPERATURE;
    } else if (offsetStr.startsWith("P")) {
        *returnOffset = BaCommunicationInterface::POWER_CONSUMPTION;
    } else if (offsetStr.startsWith("StartW")) {
        *returnOffset = BaCommunicationInterface::START_WITH_SETPOINT;
    } else if (offsetStr.startsWith("Star")) {
        *returnOffset = BaCommunicationInterface::START;
    } else if (offsetStr.startsWith("Sto")) {
        *returnOffset = BaCommunicationInterface::STOP;
    } else if (offsetStr.startsWith("Se")) {
        *returnOffset = BaCommunicationInterface::SET_CONTROLLER_CONFIGURATIONS;
    } else {
        switch (*returnControllerType) {
            case BaCommunicationInterface::AIR_CONDITIONER:
                if (offsetStr.startsWith("HumidityS")) {
                    *returnOffset =
                            BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY_SETPOINT;
                } else if (offsetStr.startsWith("H")) {
                    *returnOffset =
                            BaCommunicationInterface::AIR_CONDITIONER_HUMIDITY;
                }
                break;
            case BaCommunicationInterface::FURNACE:
                if (offsetStr.startsWith("G")) {
                    *returnOffset = BaCommunicationInterface::FURNACE_GAS_FLOW;
                }
                break;
        }
    }
}

Structure* BaIODataProviderPrivate::getControllerConfigurations() const {
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

void BaIODataProviderPrivate::setControllerConfigurations(const Structure& controllerConfigurations) {
    //TODO
}
