#include "MessageDeserializerException.h"
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

using namespace CommonNamespace;

MessageDeserializerException::MessageDeserializerException(
		const std::string& message, const std::string& file, int line) :
		Exception(message, file, line) {
}

Exception* MessageDeserializerException::copy() const {
	return new MessageDeserializerException(*this);
}
