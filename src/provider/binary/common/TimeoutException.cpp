#include "TimeoutException.h"

using namespace CommonNamespace;

TimeoutException::TimeoutException(const std::string& message,
		const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* TimeoutException::copy() const {
	return new TimeoutException(*this);
}
