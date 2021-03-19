#ifndef PROVIDER_BINARY_COMMON_CONVERSIONEXCEPTION_H
#define PROVIDER_BINARY_COMMON_CONVERSIONEXCEPTION_H

#include <common/Exception.h>
#include <string>

class ConversionException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	ConversionException(const std::string& message, const std::string& file,
			int line);

	// interface Exception
	virtual CommonNamespace::Exception* copy() const;
};

#endif /* PROVIDER_BINARY_COMMON_CONVERSIONEXCEPTION_H */
