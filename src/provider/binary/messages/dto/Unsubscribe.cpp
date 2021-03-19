#include "Unsubscribe.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

Unsubscribe::Unsubscribe(unsigned long messageId, const ParamId& paramId,
		bool attachValues) :
		ParamIdMessage(MessageHeader::UNSUBSCRIBE, messageId, paramId,
				attachValues) {
}

Unsubscribe::Unsubscribe(const Unsubscribe& orig) :
		ParamIdMessage(orig) {
}

Message* Unsubscribe::copy() const {
	return new Unsubscribe(*this);
}
