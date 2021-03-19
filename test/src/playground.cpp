#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/Variant.h>
#include <uanodeid.h> // UaNodeId
#include <uastructuredefinition.h> // UaStructureDefinition
#include <uavariant.h> // UaVariant
#include <common/Exception.h>
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <sasModelProvider/base/ConverterUa2IO.h>
#include "../../src/provider/binary/messages/ConverterBin2IO.h"

using namespace CommonNamespace;

class ConverterCallback: public SASModelProviderNamespace::ConverterUa2IO::ConverterCallback {
public:

	virtual void addStructureDefinition(
			UaStructureDefinition& structureDefinition) {
		std::string dataTypeId(
				structureDefinition.dataTypeId().toXmlString().toUtf8());
		structureDefinitions[dataTypeId] = &structureDefinition;
	}

	virtual void addDataTypeParents(UaNodeId& dataTypeId,
			std::vector<UaNodeId>& parents) {
		std::string dataTypeIdStr(dataTypeId.toXmlString().toUtf8());
		dataTypeParents[dataTypeIdStr] = &parents;
	}

	// interface Converter::ConverterCallback

	virtual UaStructureDefinition getStructureDefinition(
			const UaNodeId& dataTypeId) {
		return *structureDefinitions.at(
				std::string(dataTypeId.toXmlString().toUtf8()));
	}

	virtual std::vector<UaNodeId>* getSuperTypes(const UaNodeId& dataTypeId) {
		std::vector<UaNodeId>* parents = dataTypeParents.at(
				std::string(dataTypeId.toXmlString().toUtf8()));
		// return a copy
		std::vector<UaNodeId>* parentsCopy = new std::vector<UaNodeId>();
		for (std::vector<UaNodeId>::const_iterator it = parents->begin();
				it != parents->end(); it++) {
			parentsCopy->push_back(*it);
		}
		return parentsCopy;
	}
private:
	std::map<std::string, UaStructureDefinition*> structureDefinitions;
	std::map<std::string, std::vector<UaNodeId>*> dataTypeParents;
};

int main(int argc, char** argv) {
	UaVariantArray test;
	UaInt32Array test2;
	test.create(2);
	test2.create(1);

	UaVariant tempValue;

	// initialize logging
	ConsoleLoggerFactory consoleLoggerFactory;
	LoggerFactory loggerFactory(consoleLoggerFactory);
	Logger* log = LoggerFactory::getLogger("main");

	test2[0] = 16;

	tempValue.setInt16(20);
	tempValue.copyTo(&test[0]);

	tempValue.setString("Hello");
	tempValue.copyTo(&test[1]);

//	tempValue.setInt32Array(test2, false);
//	tempValue.copyTo(&test[2]);


	ConverterCallback callback;
	SASModelProviderNamespace::ConverterUa2IO conv(callback);

	UaVariant v;
	v.setVariantArray(test, false);

	UaVariant v2;
	v2.setInt32Array(test2, false);

	try {
		log->info("%s",
				conv.convertUa2io(v, UaNodeId(OpcUaType_Variant))->toString().c_str());
		ConverterBin2IO converterIo2bin;
		log->info("%s", converterIo2bin.convertIo2bin(*conv.convertUa2io(v, UaNodeId(OpcUaType_Variant)))->toString().c_str());


	} catch (Exception& e) {
		std::string st;
		e.getStackTrace(st);
		log->error("Exception: %s", st.c_str());
		return 1;
	}

	return 0;
}
