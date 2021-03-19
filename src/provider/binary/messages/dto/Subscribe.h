#ifndef PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBE_H
#define	PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBE_H

#include "ParamIdMessage.h"

class Subscribe: public ParamIdMessage {
public:
	Subscribe(unsigned long messageId, const ParamId& paramId,
			bool attachValues = false);
	// Creates a deep copy of the instance.
	Subscribe(const Subscribe& orig);

	// interface Message
	virtual Message* copy() const;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBE_H */

