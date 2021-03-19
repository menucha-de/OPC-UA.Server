#include <ioDataProvider/IODataProviderException.h>

namespace IODataProviderNamespace {

using namespace CommonNamespace;

IODataProviderException::IODataProviderException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* IODataProviderNamespace::IODataProviderException::copy() const {
	return new IODataProviderException(*this);
}

} // namespace IODataProviderNamespace
