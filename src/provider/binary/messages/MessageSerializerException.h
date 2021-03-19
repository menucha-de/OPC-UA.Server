#ifndef PROVIDER_BINARY_MESSAGES_MESSAGESERIALIZEREXCEPTION_H
#define PROVIDER_BINARY_MESSAGES_MESSAGESERIALIZEREXCEPTION_H

#include <common/Exception.h>
#include <string>

class MessageSerializerException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	MessageSerializerException(const std::string& message,
			const std::string& file, int line);

	// interface Exception
	virtual CommonNamespace::Exception* copy() const;
};

#endif /* PROVIDER_BINARY_MESSAGES_MESSAGESERIALIZEREXCEPTION_H */
