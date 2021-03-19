#include "UnsubscribeResponse.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

UnsubscribeResponse::UnsubscribeResponse(unsigned long messageId,
		Status::Value status) :
		StatusMessage(MessageHeader::UNSUBSCRIBE_RESPONSE, messageId, status) {
}

UnsubscribeResponse::UnsubscribeResponse(const UnsubscribeResponse& orig) :
		StatusMessage(orig) {
}

Message* UnsubscribeResponse::copy() const {
	return new UnsubscribeResponse(*this);
}
