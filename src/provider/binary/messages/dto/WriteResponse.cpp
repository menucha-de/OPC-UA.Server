#include "WriteResponse.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

WriteResponse::WriteResponse(unsigned long messageId, Status::Value status) :
		StatusMessage(MessageHeader::WRITE_RESPONSE, messageId, status) {
}

WriteResponse::WriteResponse(const WriteResponse& orig) :
		StatusMessage(orig) {
}

Message* WriteResponse::copy() const {
	return new WriteResponse(*this);
}
