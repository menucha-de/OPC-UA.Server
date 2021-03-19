#include "HaNodeManagerNodeSetXmlException.h"

using namespace CommonNamespace;

HaNodeManagerNodeSetXmlException::HaNodeManagerNodeSetXmlException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* HaNodeManagerNodeSetXmlException::copy() const {
	return new HaNodeManagerNodeSetXmlException(*this);
}
