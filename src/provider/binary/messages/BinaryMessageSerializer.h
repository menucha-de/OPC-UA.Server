#ifndef PROVIDER_BINARY_MESSAGES_BINARYMESSAGESERIALIZER_H
#define	PROVIDER_BINARY_MESSAGES_BINARYMESSAGESERIALIZER_H

#include "dto/Message.h"

class BinaryMessageSerializerPrivate;

class BinaryMessageSerializer {
public:
	static const int MESSAGE_HEADER_LENGTH = 10;

	BinaryMessageSerializer();
	virtual ~BinaryMessageSerializer();

	// Serializes a message to a byte array.
	// The returned array must be deleted by the caller.
	virtual unsigned char* serialize(
			Message& message) /* throws MessageSerializerException */;
private:
	BinaryMessageSerializerPrivate* d;
};

#endif	/* PROVIDER_BINARY_MESSAGES_BINARYMESSAGESERIALIZER_H */

