#ifndef PROVIDER_BINARY_MESSAGES_DTO_UNSUBSCRIBE_H
#define	PROVIDER_BINARY_MESSAGES_DTO_UNSUBSCRIBE_H

#include "ParamIdMessage.h"

class Unsubscribe: public ParamIdMessage {
public:
	Unsubscribe(unsigned long messageId, const ParamId& paramId,
			bool attachValues = false);
	// Creates a deep copy of the instance.
	Unsubscribe(const Unsubscribe& orig);

	// interface ParamIdMessage
	virtual Message* copy() const;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBE_H */

