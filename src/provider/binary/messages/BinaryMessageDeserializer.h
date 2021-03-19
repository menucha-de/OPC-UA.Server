#ifndef PROVIDER_BINARY_MESSAGES_BINARYMESSAGEDESERIALIZER_H
#define	PROVIDER_BINARY_MESSAGES_BINARYMESSAGEDESERIALIZER_H

#include "dto/Message.h"
#include "dto/MessageHeader.h"

class BinaryMessageDeserializerPrivate;

class BinaryMessageDeserializer {
public:
	BinaryMessageDeserializer();
	virtual ~BinaryMessageDeserializer();

	// Deserializes a byte array to a message header.
	// The returned object must be deleted by the caller. The object is
	// responsible for destroying its own sub structures.
	virtual MessageHeader* deserializeMessageHeader(unsigned char* buffer);
	// Deserializes a byte array to a message body.
	// The given messageHeader reference is neither stored internally
	// nor part of the returned object.
	// The returned object must be deleted by the caller. The object is
	// responsible for destroying its own sub structures.
	virtual Message* deserializeMessageBody(const MessageHeader& messageHeader,
			unsigned char* buffer) /* throws MessageDeserializerException */;
private:
	BinaryMessageDeserializerPrivate* d;
};

#endif	/* PROVIDER_BINARY_MESSAGES_BINARYMESSAGEDESERIALIZER_H */

