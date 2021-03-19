#include <sasModelProvider/base/NodeBrowserException.h>

using namespace CommonNamespace;

namespace SASModelProviderNamespace {

NodeBrowserException::NodeBrowserException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* NodeBrowserException::copy() const {
	return new NodeBrowserException(*this);
}

} // namespace SASModelProviderNamespace
