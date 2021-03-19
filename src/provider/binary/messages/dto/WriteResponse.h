#ifndef PROVIDER_BINARY_MESSAGES_DTO_WRITERESPONSE_H
#define	PROVIDER_BINARY_MESSAGES_DTO_WRITERESPONSE_H

#include "StatusMessage.h"

class WriteResponse: public StatusMessage {
public:
	WriteResponse(unsigned long messageId, Status::Value status);
	// Creates a deep copy of the instance.
	WriteResponse(const WriteResponse& orig);

	// interface StatusMessage
	virtual Message* copy() const;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_READRESPONSE_H */

