#ifndef PROVIDER_BINARY_MESSAGES_DTO_READ_H
#define	PROVIDER_BINARY_MESSAGES_DTO_READ_H

#include "ParamIdMessage.h"

class Read: public ParamIdMessage {
public:
	Read(unsigned long messageId, ParamId& paramId, bool attachValues = false);
	// Creates a deep copy of the instance.
	Read(const Read& orig);

	// interface ParamIdMessage
	virtual Message* copy() const;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_READ_H */

