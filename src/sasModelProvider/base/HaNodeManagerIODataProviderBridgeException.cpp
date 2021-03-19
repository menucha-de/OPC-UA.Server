#include <sasModelProvider/base/HaNodeManagerIODataProviderBridgeException.h>

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

HaNodeManagerIODataProviderBridgeException::HaNodeManagerIODataProviderBridgeException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* HaNodeManagerIODataProviderBridgeException::copy() const {
	return new HaNodeManagerIODataProviderBridgeException(*this);
}

} // namespace SASModelProviderNamespace
