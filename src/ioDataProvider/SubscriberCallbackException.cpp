#include <ioDataProvider/SubscriberCallbackException.h>

namespace IODataProviderNamespace {

using namespace CommonNamespace;

SubscriberCallbackException::SubscriberCallbackException(
		const std::string& message, const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* SubscriberCallbackException::SubscriberCallbackException::copy() const {
	return new SubscriberCallbackException(*this);
}

} // namespace IODataProviderNamespace
