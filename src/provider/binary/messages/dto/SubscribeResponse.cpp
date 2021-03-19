#include "SubscribeResponse.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

SubscribeResponse::SubscribeResponse(unsigned long messageId,
		Status::Value status) :
		StatusMessage(MessageHeader::SUBSCRIBE_RESPONSE, messageId, status) {
}

SubscribeResponse::SubscribeResponse(const SubscribeResponse& orig) :
		StatusMessage(orig) {
}

Message* SubscribeResponse::copy() const {
	return new SubscribeResponse(*this);
}
