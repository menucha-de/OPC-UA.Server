#ifndef PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBERESPONSE_H
#define	PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBERESPONSE_H

#include "StatusMessage.h"

class SubscribeResponse: public StatusMessage {
public:
	SubscribeResponse(unsigned long messageId, Status::Value status);
	// Creates a deep copy of the instance.
	SubscribeResponse(const SubscribeResponse& orig);

	// interface StatusMessage
	virtual Message* copy() const;
};

#endif	/* PROVIDER_BINARY_MESSAGES_DTO_SUBSCRIBERESPONSE_H */

