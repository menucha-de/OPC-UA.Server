#ifndef PROVIDER_BINARY_MESSAGES_MESSAGEDESERIALIZEREXCEPTION_H_
#define PROVIDER_BINARY_MESSAGES_MESSAGEDESERIALIZEREXCEPTION_H_

#include <common/Exception.h>
#include <string>

class MessageDeserializerException: public CommonNamespace::Exception {
public:
	// Copies of the given message and file path are saved internally.
	MessageDeserializerException(const std::string& message,
			const std::string& file, int line);

	// interface Exception
	virtual CommonNamespace::Exception* copy() const;
};

#endif /* PROVIDER_BINARY_MESSAGES_MESSAGEDESERIALIZEREXCEPTION_H_ */
