#include "ClientSocketException.h"

using namespace CommonNamespace;

ClientSocketException::ClientSocketException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* ClientSocketException::copy() const {
	return new ClientSocketException(*this);
}
