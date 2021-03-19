#ifndef PROVIDER_BINARY_MESSAGES_DTO_UNSUBSCRIBERESPONSE_H
#define	PROVIDER_BINARY_MESSAGES_DTO_UNSUBSCRIBERESPONSE_H

#include "StatusMessage.h"

class UnsubscribeResponse: public StatusMessage {
public:
	UnsubscribeResponse(unsigned long messageId, Status::Value status);
	// Creates a deep copy of the instance.
	UnsubscribeResponse(const UnsubscribeResponse& orig);

	// interface StatusMessage
	virtual Message* copy() const;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_UNSUBSCRIBERESPONSE_H */

