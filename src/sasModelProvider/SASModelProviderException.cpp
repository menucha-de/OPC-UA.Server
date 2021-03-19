#include <sasModelProvider/SASModelProviderException.h>

namespace SASModelProviderNamespace {

using namespace CommonNamespace;

SASModelProviderException::SASModelProviderException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* SASModelProviderException::copy() const {
	return new SASModelProviderException(*this);
}

} // namespace SASModelProviderNamespace
