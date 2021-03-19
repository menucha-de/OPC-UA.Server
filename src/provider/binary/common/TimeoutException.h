#ifndef PROVIDER_BINARY_COMMON_TIMEOUTEXCEPTION_H
#define PROVIDER_BINARY_COMMON_TIMEOUTEXCEPTION_H

#include <common/Exception.h>
#include <string>

class TimeoutException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	TimeoutException(const std::string& message, const std::string& file,
			int line);

	// interface Exception
	virtual CommonNamespace::Exception* copy() const;
};

#endif /* PROVIDER_BINARY_COMMON_TIMEOUTEXCEPTION_H */
