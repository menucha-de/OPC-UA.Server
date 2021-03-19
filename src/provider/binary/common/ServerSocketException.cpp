#include "ServerSocketException.h"

using namespace CommonNamespace;

ServerSocketException::ServerSocketException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* ServerSocketException::copy() const {
	return new ServerSocketException(*this);
}
