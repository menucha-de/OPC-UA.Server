#ifndef PROVIDER_BINARY_MESSAGES_DTO_MESSAGE_H
#define	PROVIDER_BINARY_MESSAGES_DTO_MESSAGE_H

#include "MessageHeader.h"

/* An interface which must be implemented by all messages. */
class Message {
public:
	Message();
	virtual ~Message();

	virtual MessageHeader& getMessageHeader() = 0;
	// Creates a deep copy of the instance.
	virtual Message* copy() const = 0;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_MESSAGE_H */

