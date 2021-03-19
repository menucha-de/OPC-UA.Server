#include "Subscribe.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

Subscribe::Subscribe(unsigned long messageId, const ParamId& paramId,
		bool attachValues) :
		ParamIdMessage(MessageHeader::SUBSCRIBE, messageId, paramId,
				attachValues) {
}

Subscribe::Subscribe(const Subscribe& orig) :
		ParamIdMessage(orig) {
}

Message* Subscribe::copy() const {
	return new Subscribe(*this);
}
