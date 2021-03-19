#include "Read.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

Read::Read(unsigned long messageId, ParamId& paramId, bool attachValues) :
		ParamIdMessage(MessageHeader::READ, messageId, paramId, attachValues) {
}

Read::Read(const Read& orig) :
		ParamIdMessage(orig) {
}

Message* Read::copy() const {
	return new Read(*this);
}
