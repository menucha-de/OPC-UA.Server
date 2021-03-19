#include <sasModelProvider/base/HaNodeManagerException.h>

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

HaNodeManagerException::HaNodeManagerException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* HaNodeManagerException::copy() const {
	return new HaNodeManagerException(*this);
}

} // namespace SASModelProviderNamespace
